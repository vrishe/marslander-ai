#pragma once

#ifndef COMMON_H_
#define COMMON_H_

#include <algorithm>
#include <cstdint>
#include <istream>
#include <iterator>
#include <vector>

namespace marslander {

typedef int32_t inum;
typedef double  fnum;

template<typename T>
struct point {
  typedef T value_type;
  value_type x,y;

  template<typename M>
  operator point<M>() const {
    return {
      static_cast<M>(x),
      static_cast<M>(y)
    };
  }
};

template<typename T>
struct span { 
  typedef T value_type;
  value_type start, end;
};

using ipoint = point<inum>;
using fpoint = point<fnum>;

using ipoints = std::vector<ipoint>;
using fpoints = std::vector<fpoint>;

struct game_init_input {
  using surface_type = ipoints;
  surface_type surface;

  using safe_area_type = span<surface_type::size_type>;
  safe_area_type safe_area;
};

struct game_turn_input {
  using scalar_type = inum;
  scalar_type fuel, thrust, tilt;

  using position_type = ipoint;
  position_type position;

  using velocity_type = fpoint;
  velocity_type velocity;
};

struct game_turn_output {
  using scalar_type = inum;
  scalar_type thrust, tilt;
};

template<typename T>
std::istream& operator>>(std::istream& stream, point<T>& p) {
  return stream >> p.x >> p.y;
}

template<typename T>
std::ostream& operator<<(std::ostream& stream, const point<T>& p) {
  return stream << "{ " << p.x << ", " << p.y << " }";
}

template<class T>
std::istream& operator>>(std::istream& stream, std::vector<T>& v) {
  size_t v_size;
  stream >> v_size; stream.ignore();

  v.resize(v_size);
  std::copy_n(std::istream_iterator<T>(stream), v_size, v.begin());

  return stream;
}

template<typename T>
std::ostream& operator<<(std::ostream& stream, const game_turn_output& g) {
  return stream << g.tilt << " " << g.thrust;
}

template<class T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& v) {
  bool first = true;
  for (auto& item : v) {
    if (!first) stream << ", ";
    first = false;
    stream << item;
  }

  return stream;
}

#ifndef __cpp_lib_interpolate

#include <type_traits>

template<typename T>
constexpr
std::enable_if_t<std::is_floating_point_v<T>, T>
lerp(T a, T b, T t) noexcept {
  if ((a <= 0 && b >= 0) || (a >= 0 && b <= 0))
    return t * b + (1 - t) * a;

  if (t == 1)
    return b; // exact

  // Exact at t=0, monotonic except near t=1,
  // bounded, determinate, and consistent:
  const T x = a + t * (b - a);
  return (t > 1) == (b > a)
    ? (b < x ? x : b)
    : (b > x ? x : b);  // monotonic near t=1
}

#else

using std::lerp;

#endif // ifndef __cpp_lib_interpolate

} // namespace marslander

#endif // COMMON_H_
