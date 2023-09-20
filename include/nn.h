#pragma once

#ifndef NN_H_
#define NN_H_

#include "common.h"
#include "constants.h"
#include "linalg.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <tuple>

namespace marslander::nn {

using namespace linalg;

template<size_t InputSize, size_t NeuronsCount>
struct layer_meta final {

  static constexpr size_t input_size = InputSize;
  static constexpr size_t neurons_count = NeuronsCount;

  static constexpr size_t layer_size = neurons_count * (input_size + 1);
  static constexpr size_t layer_weights_offset = neurons_count;

};

template<typename T, typename TMeta>
struct layer final {

  typedef T value_type;
  typedef TMeta meta_type;

  typedef matrix<value_type, meta_type::neurons_count, 1> bias_type;
  typedef matrix<value_type, meta_type::neurons_count,
    meta_type::input_size> weights_type;

  bias_type B;
  weights_type W;

  typedef matrix<value_type, meta_type::input_size, 1> in_type;
  typedef matrix<value_type, meta_type::neurons_count, 1> out_type;

  out_type operator() (const in_type& prevA) const {
    return mul(W, prevA) + B;
  }
};

namespace activators {

template<typename T>
struct ReLU {
  T operator() (T value) { return std::max(T(0), value); }
};

template<typename T>
struct sigmoid {
  T operator() (T value) { return 1 / (1 + std::exp(-value)); }
};

template<typename T>
struct tanh {
  T operator() (T value) { return std::tanh(value); }
};

} // namespace activators

struct DFF_meta final {

  using hidden0_meta = layer_meta<7, 5>;
  using hidden1_meta = layer_meta<5, 3>;
  using output_meta  = layer_meta<3, 2>;

  static constexpr size_t total_size
    = hidden0_meta::layer_size
    + hidden1_meta::layer_size
    + output_meta ::layer_size;

};

template<typename T>
struct DFF_base {

  /*
   * 7-vector input:
   * thrust
   * tilt
   * position_x (normalize around center of landing pad)
   * position_y (normalize against elevation of landing pad)
   * abs(velocity_x) is out of landing allowed value
   * abs(velocity_y) is out of landing allowed value
   * obstacle   (raycast check, whether shuttle has an obstacle on its way)
   *
   * 2-vector output:
   * thrust
   * tilt
   */

  typedef DFF_meta meta_type;
  typedef T value_type;

  layer<value_type, meta_type::hidden0_meta> hidden0;
  layer<value_type, meta_type::hidden1_meta> hidden1;
  layer<value_type, meta_type::output_meta > output;

  typedef typename decltype(DFF_base::hidden0)::in_type in_type;
  typedef typename decltype(DFF_base::output)::out_type out_type;

};

template<typename T, class Activator = activators::ReLU<T>>
struct DFF final : DFF_base<T> {

  using base_ = DFF_base<T>;

  typename base_::out_type operator() (
      const typename base_::in_type& input) const {
    auto g = Activator();
    return apply(g, base_::output(
      apply(g, base_::hidden1(
        apply(g, base_::hidden0(input))
      ))
    ));
  }

};

namespace detail_ {

template<typename T>
using vec3 = matrix<T, 3>;

template<typename T>
inline vec3<T> cross(const vec3<T>& a, const vec3<T>& b) {
  return {
    a.value_at(1) * b.value_at(2) - a.value_at(2) * b.value_at(1),
    a.value_at(2) * b.value_at(0) - a.value_at(0) * b.value_at(2),
    a.value_at(0) * b.value_at(1) - a.value_at(1) * b.value_at(0),
  };
}

template<typename T>
inline vec3<T> line(const point<T>& a, const point<T>& b) {
  return {b.y - a.y, a.x - b.x, a.x * b.y - b.x * a.y};
}

template<typename T>
inline point<T> as_point(const vec3<T>& hp) {
  auto z = hp.value_at(2);
  return { hp.value_at(0) / z, hp.value_at(1) / z };
}

template<typename T>
inline T dot(const point<T>& u, const point<T>& v) {
  return u.x * v.x + u.y * v.y;
}

template<typename T>
inline point<T> operator+(const point<T>& a, const point<T>& b) {
  return { a.x+b.x, a.y+b.y };
}

template<typename T>
inline point<T> operator-(const point<T>& a, const point<T>& b) {
  return { a.x-b.x, a.y-b.y };
}

} // namespace detail_

template<typename T, class Activator>
class game_adapter final {

  using dff_t = DFF<T, Activator>;
  using target_in_t = typename dff_t::value_type;
  using target_out_t = game_turn_output::scalar_type;

  const dff_t& _dff;
  const game_init_input& _in;
  const game_turn_input& _t0;

  using surface_point_t = game_init_input::surface_type::value_type;
  using surface_index_t = game_init_input::surface_type::size_type;
  const span<surface_point_t::value_type> _safe_area_x;
  const surface_point_t::value_type _safe_area_alt;
  const target_in_t _safe_area_elev;

  using line_t = detail_::vec3<game_turn_input::velocity_type::value_type>;
  using point_t = point<line_t::value_type>;
  std::vector<std::tuple<line_t, point_t, point_t>> _lines;

public:

  game_adapter(
    const dff_t& dff,
    const game_init_input& game_init,
    const game_turn_input& turn_zero)
    : _dff(dff),
      _in(game_init),
      _t0(turn_zero),
      _safe_area_x {
        _in.surface[_in.safe_area.start].x,
        _in.surface[_in.safe_area.end].x
      },
      _safe_area_alt(_in.surface[_in.safe_area.start].y),
      _safe_area_elev(_t0.position.y - _safe_area_alt)
    {
      auto l = std::back_inserter(_lines);
      for (surface_index_t i = 1, imax = _in.surface.size(); i < imax; ++i) {
        point_t a = _in.surface[i - 1], b = _in.surface[i];
        *l++ = std::make_tuple(detail_::line(a, b),a,b);
      }
    }

  auto check_obstacle(const game_turn_input& turn) const {
    using namespace detail_;
    auto pos = static_cast<point_t>(turn.position);
    auto ray = line(pos, pos + turn.velocity);

    auto sqr_dst_min = std::numeric_limits<point_t::value_type>::max();
    for (const auto& t : _lines) {
      auto [l, ia, ib] = t;
      auto p = as_point(cross(l, ray));

      point_t d;
      if (std::isnan(p.x + p.y)
        || dot(turn.velocity, d = p-pos) < 0
        || dot(p-ia,ib-ia) < 0
        || dot(p-ib,ia-ib) < 0) continue;

      auto sqr_dst = dot(d, d);
      if (sqr_dst < sqr_dst_min)
        sqr_dst_min = sqr_dst;
    }
    // seconds ^ -1
    return std::sqrt(dot(turn.velocity, turn.velocity) / sqr_dst_min);
  }

  game_turn_output get_output(const game_turn_input& turn) const {
    constexpr auto deg2rad = M_PI / 180.;
    auto dff_output = _dff({
      static_cast<target_in_t>(turn.thrust) / constants::thrust_power_max,
      static_cast<target_in_t>(std::sin(turn.tilt * deg2rad)),
      static_cast<target_in_t>(std::max(
        _safe_area_x.start - turn.position.x,
        turn.position.x - _safe_area_x.end))
          / constants::zone_width,
      static_cast<target_in_t>(turn.position.y - _safe_area_alt)
          / _safe_area_elev,
      static_cast<target_in_t>(std::abs(turn.velocity.x)
        >= constants::speed_limit_horz),
      static_cast<target_in_t>(std::abs(turn.velocity.y)
        >= constants::speed_limit_vert),
      static_cast<target_in_t>(check_obstacle(turn)),
    });

    constexpr auto rad2deg = 180. / M_PI;
    return {
      static_cast<target_out_t>(std::round(constants::thrust_power_max
        * std::clamp<target_in_t>(dff_output.value_at(0), 0, 1))),
      static_cast<target_out_t>(std::round(rad2deg
        * std::asin(std::clamp<target_in_t>(dff_output.value_at(1), -1, 1)))),
    };
  }

};

} // namespace marslander::nn

#endif // NN_H_
