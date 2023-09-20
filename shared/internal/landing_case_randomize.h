#include "proto/landing_case.pb.h"
#include "constants.h"

#include <cmath>
#include <functional>
#include <random>

namespace marslander::landing_case {

inline constexpr fnum elevation_C = fnum(2.2);

inline constexpr inum fuel_B = 550;
inline constexpr fnum fuel_D = 200;
inline constexpr fnum fuel_K = 23.07;

inline constexpr fnum initial_speed_max = 100;  // m/s
inline constexpr fnum initial_speed_min = 0;    // m/s

inline constexpr inum start_position_altitude_max = 2800; // m
inline constexpr inum start_position_altitude_min = 2700; // m
inline constexpr inum start_position_horz_padding = 500;  // m
inline constexpr inum surface_elevation_max       = 2800; // m
inline constexpr inum surface_flat_elevation_max  = 2100; // m
inline constexpr inum surface_flat_elevation_min  = 100;  // m
inline constexpr inum surface_flat_width_max      = 2000; // m
inline constexpr inum surface_flat_width_step     = 500; // m
inline constexpr inum surface_points_count_min    = 4;    //
inline constexpr inum surface_points_count_max    = 25;   //

inline constexpr inum zone_horz_padding = 500; // m

namespace detail_ {

template <typename T, size_t N>
constexpr size_t countof(T const (&)[N]) noexcept { return N; }

using namespace std;

template<typename T, typename Cmp = less<>>
bool check_range(T a, T b, T v, Cmp cmp = Cmp{}) {
  return cmp(a, v) && cmp(v, b);
}

template<typename T>
auto sign(T val) { return (T(0) < val) - (val < T(0)); }

template<typename Rng, typename D>
auto next_val(Rng&& rng,
    typename D::result_type a, typename D::result_type b) {
  return D(a, b)(rng);
}

template<typename Rng>
auto next_int_val(Rng&& rng, inum a, inum b) {
  return next_val<Rng, uniform_int_distribution<inum>>(rng, a, b);
}

template<typename Rng>
auto next_real_val(Rng&& rng, fnum a, fnum b) {
  return next_val<Rng, uniform_real_distribution<fnum>>(rng, a, 
    nextafter(b, numeric_limits<fnum>::max()));
}

inline auto elevation_curve(inum t0, inum t1, fnum v) {
  v = elevation_C * (v - t0) / (t1 - t0) + 1;
  return 1 - 1 / (v*v);
}

inline void set_surface_point(pb::point_int32* p, inum x, inum y) {
  p->set_x(x);
  p->set_y(y);
}

template<typename Surf>
fnum surface_level(const Surf& surface, inum x) {
  auto hi = upper_bound(surface.begin(), surface.end(), x,
    [](auto value, const auto& p) {
      return p.x() > value; 
    });

  auto lo = hi-1;
  return lo->y() + (x - lo->x())
    * (hi->y() - lo->y()) / fnum(hi->x() - lo->x());
}

template<typename Rng>
void get_flat(Rng&& rng, inum& flat_start, inum& flat_end,
    inum& flat_elevation) {

  auto flat_width = constants::surface_flat_width_min
    + surface_flat_width_step * next_int_val(rng, 0,
        (surface_flat_width_max - constants::surface_flat_width_min)
          / surface_flat_width_step);

  auto ofs = (flat_width >> 1) + (flat_width & 1);
  auto cx = next_int_val(rng, ofs, constants::zone_x_max - ofs);

  if (cx <= constants::zone_x_max / 2) {
    flat_start = cx - ofs;
    flat_end = flat_start + flat_width;
  }
  else {
    flat_end = cx + ofs;
    flat_start = flat_end - flat_width;
  }

  flat_elevation = next_int_val(rng,
    surface_flat_elevation_min,
    surface_flat_elevation_max);
}

template<typename Rng>
void fill_surface(Rng&& rng, pb::landing_case& test_case,
    inum flat_start, inum flat_end, inum flat_elevation) {

  test_case.clear_surface();

  auto surface_size = next_int_val(rng,
    surface_points_count_min + 1, surface_points_count_max);
  auto step = constants::zone_x_max / fnum(surface_size - 1);
  auto step_2 = step / 2;
  auto step_6 = step / 6;

  auto ndist = normal_distribution<fnum>{0, step_6};
  auto udist = uniform_real_distribution<fnum>{0, 1};

  test_case.clear_safe_area();
  auto safe_area = test_case.mutable_safe_area();

  auto surface = test_case.mutable_surface();
  surface->Reserve(surface_size + 1);

  enum { start, flat, end } state{};
  for (auto i = 0, imax = surface_size - 1; i <= imax; ++i) {
    auto x = inum(round(step * i 
      + (
          check_range(0, imax, i) && step_2 > 1
            ? clamp(ndist(rng), -step_2+1, step_2-1)
            : fnum(0)
        )
    ));

    switch (state) {
      case start: {
        if (x >= flat_start) {
          state = flat;
          safe_area->set_start(test_case.surface_size());
          set_surface_point(surface->Add(),
            flat_start, flat_elevation);

          if (i == imax) {
            safe_area->set_end(test_case.surface_size());
            set_surface_point(surface->Add(),
              flat_end, flat_elevation);

            if (x > flat_end)
              break;
          }

          continue;
        }
        break;
      }
      case flat: {
        if (x >= flat_end) {
          state = end;
          safe_area->set_end(test_case.surface_size());
          set_surface_point(surface->Add(),
            flat_end, flat_elevation);

          if (i == imax && x > flat_end)
            break;
        }
        continue;
      }
    }

    auto y = inum(round(
      surface_elevation_max * udist(rng)
        * (
            x < flat_start ? elevation_curve(flat_start, 0, x)
          : x > flat_end   ? elevation_curve(flat_start,
                               constants::zone_x_max, x)
          : 0
        )
    ));

    set_surface_point(surface->Add(), x, y);
  }
}

template<typename Rng>
void fill_position(Rng&& rng, pb::landing_case& test_case,
    inum flat_start, inum flat_end) {

  auto position = test_case.mutable_position();
  auto surface = test_case.surface();
  auto steps =  0;
  do {

    if (++steps > 16) { // tie-break
      position->set_x(next_int_val(rng, flat_start, flat_end));
    }
    else {
      position->set_x(next_int_val(rng,
        zone_horz_padding,
        constants::zone_x_max - zone_horz_padding));
    }

    position->set_y(next_int_val(rng,
      start_position_altitude_min,
      start_position_altitude_max));
  }
  while (position->y() <= surface_level(surface, position->x()));
}

} // namespace detail_

template<typename Rng>
void randomize(Rng&& rng, pb::landing_case& test_case) {
  using namespace detail_;

  test_case.set_thrust(constants::thrust_power_min);

  inum flat_start, flat_end, flat_elevation;
  get_flat(rng, flat_start, flat_end, flat_elevation);

  fill_surface(rng, test_case, flat_start, flat_end, flat_elevation);
  fill_position(rng, test_case, flat_start, flat_end);

  test_case.set_fuel(max(
    fuel_B + inum(fuel_K * (test_case.surface_size() - 7)
      + normal_distribution<fnum>(fnum(0), fuel_D)(rng)), 
    constants::fuel_amount_max));

  {
    inum tilt_spec[] = {
      constants::tilt_angle_min, 
      0, 0, 0, 0,
      constants::tilt_angle_max
    };
    auto tilt = tilt_spec[next_int_val(
      rng, 0, countof(tilt_spec) - 1)];
    test_case.set_tilt(tilt);
  }

  {
    auto px = test_case.position().x();
    auto velocity = test_case.mutable_velocity();
    velocity->set_x(!check_range<inum, less_equal<>>(flat_start, flat_end, px)
        && bernoulli_distribution(.8)(rng)
      ? sign((flat_start + flat_end)/2 - px)
        * next_int_val(rng, initial_speed_min, initial_speed_max)
      : 0);
    velocity->set_y(0);
  }
}

} // namespace marslander
