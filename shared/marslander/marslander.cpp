#define _USE_MATH_DEFINES
#include "marslander.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace marslander {

using namespace std;

static void apply_state_changes(state& state);
static fnum surface_level(const state::surface_type& surface,
  state::position_type::value_type x,
  state::surface_type::value_type& out_line_start,
  state::surface_type::value_type& out_line_end);

template<typename TTo, typename TFrom>
static inline TTo inner_round(TFrom v);

template<typename P0, typename P1>
static inline P0 intersect(const P0& l1_start, const P0& l1_end,
    const P1& l2_start, const P1& l2_end);

outcome simulate(state& state) {
  auto positionPrev = state.position;
  apply_state_changes(state);

  if (0 > state.position.x || state.position.x >= constants::zone_width
      || 0 > state.position.y || state.position.y >= constants::zone_height)
    return outcome::Lost;

  state::surface_type::value_type line_start, line_end;
  auto h = surface_level(state.surface, state.position.x, line_start, line_end);
  if (state.position.y > h) return outcome::Aerial;

  auto landed = state.tilt == 0
    // Within landing area
    && (state.safe_area_x.start <= state.position.x
      && state.position.x < state.safe_area_x.end)
    // Speed vectors stay within limits
    && abs(state.velocity.x) <= constants::speed_limit_horz
    && state.velocity.y >= -constants::speed_limit_vert
    // Landing area surface is close and is approached from above
    && state.velocity.y < 0
    && (state.safe_area_alt <= (state.position.y - .5 * state.velocity.y)
      && (state.position.y + .5 * state.velocity.x) <= state.safe_area_alt);

  if (!landed) return outcome::Crashed;
  state.position = intersect(positionPrev, state.position, line_start, line_end);
  return outcome::Landed;
}

void apply_state_changes(state& state) {
  state.thrust = clamp(state.thrust
    + clamp(state.out.thrust - state.thrust,
          -constants::thrust_delta_abs, constants::thrust_delta_abs),
    constants::thrust_power_min, constants::thrust_power_max);

  state.tilt = clamp(state.tilt
    + clamp(state.out.tilt - state.tilt,
        -constants::tilt_delta_abs, constants::tilt_delta_abs),
    constants::tilt_angle_min, constants::tilt_angle_max);

  state.fuel -= state.thrust;
  if (state.fuel <= 0) {
    state.fuel = 0;
    state.thrust = 0;
  }

  using fp_local = state::velocity_type::value_type;
  fp_local tilt_rad = state.tilt * M_PI / 180.;
  fp_local aX = -sin(tilt_rad) * state.thrust;
  fp_local aY =  cos(tilt_rad) * state.thrust + constants::mars_gravity_acc;

  using pos_local = state::position_type::value_type;
  state.position.x = state.position.x + inner_round<pos_local>(state.velocity.x + fp_local(.5)*aX);
  state.position.y = state.position.y + inner_round<pos_local>(state.velocity.y + fp_local(.5)*aY);

  state.velocity.x = state.velocity.x + aX;
  state.velocity.y = state.velocity.y + aY;
}

fnum surface_level(const state::surface_type& surface,
    state::position_type::value_type x,
    state::surface_type::value_type& out_line_start,
    state::surface_type::value_type& out_line_end) {

  if (surface.size() < 2)
    throw domain_error("Given surface doesn't contain enough points (at least 2 needed).");

  auto first = surface.begin();
  if (x <= first->x) {
    out_line_start = *first;
    out_line_end = *(first+1);
    return first->y;
  }

  auto last  = surface.end()-1;
  if (x >= last->x) {
    out_line_start = *(last-1);
    out_line_end = *last;
    return last->y;
  }

  using point_type = state::surface_type::value_type;
  auto hi = upper_bound(first, ++last, x,
    [](auto value, const auto& p) {
      return p.x > value; 
    });
  auto lo = hi-1;

  out_line_start = *lo;
  out_line_end = *hi;
  return lo->y + (x - lo->x) * (hi->y - lo->y) / fnum(hi->x - lo->x);
}

template<typename TTo, typename TFrom>
inline TTo inner_round(TFrom v) { return TTo(lround(v)); }

template<typename P0, typename P1>
inline P0 intersect(const P0& l1_start, const P0& l1_end,
    const P1& l2_start, const P1& l2_end) {

  auto a1 = l1_start.y - l1_end.y;
  auto a2 = l1_end.x - l1_start.x;
  auto a3 = l1_start.x*l1_end.y - l1_end.x*l1_start.y;

  auto b1 = l2_start.y - l2_end.y;
  auto b2 = l2_end.x - l2_start.x;
  auto b3 = l2_start.x*l2_end.y - l2_end.x*l2_start.y;

  auto cx = a2*b3-b2*a3;
  auto cy = b1*a3-a1*b3;
  fnum cz = a1*b2-b1*a2;

  return {
    inner_round<typename P0::value_type>(cx / cz),
    inner_round<typename P0::value_type>(cy / cz)
  };
}

} // namespace marslander
