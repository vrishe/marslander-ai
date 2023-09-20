#pragma once

#ifndef LINALG_H_
#define LINALG_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <type_traits>

namespace marslander::linalg {

namespace detail_ {

// For convenient trailing-return-types in C++11:
#define AUTO_RETURN(...) \
 noexcept(noexcept(__VA_ARGS__)) -> decltype(__VA_ARGS__) {return (__VA_ARGS__);}

template <typename T>
constexpr auto decayed_begin(T&& c)
AUTO_RETURN(std::begin(std::forward<T>(c)))

template <typename T>
constexpr auto decayed_end(T&& c)
AUTO_RETURN(std::end(std::forward<T>(c)))

template <typename T, std::size_t N>
constexpr auto decayed_begin(T(&c)[N])
AUTO_RETURN(reinterpret_cast<typename std::remove_all_extents_t<T>*>(c    ))

template <typename T, std::size_t N>
constexpr auto decayed_end(T(&c)[N])
AUTO_RETURN(reinterpret_cast<typename std::remove_all_extents_t<T>*>(c + N))

} // namespace detail_

template<typename T, size_t Rows, size_t Cols = 1>
struct matrix {

  typedef matrix<T, Rows, 1> col_type;
  typedef matrix<T, 1, Cols> row_type;
  typedef T value_type;

  matrix(std::initializer_list<value_type> l) {
    auto src_it = l.begin(),
         src_end = l.end();
    auto dst_it = detail_::decayed_begin(_values),
         dst_end = detail_::decayed_end(_values);

    while (src_it != src_end && dst_it != dst_end)
      *dst_it++ = *src_it++;
    while (dst_it != dst_end)
      *dst_it++ = 0;
  }

  matrix(const matrix<value_type, Rows, Cols>& other) {
    std::copy(detail_::decayed_begin(other._values),
      detail_::decayed_end(other._values), detail_::decayed_begin(_values));
  }

  constexpr size_t cols() const { return Cols; }
  constexpr size_t rows() const { return Rows; }

  const value_type& value_at (size_t r, size_t c = 0) const { return _values[r][c]; }
        value_type& value_at (size_t r, size_t c = 0)       { return _values[r][c]; }

  auto col(size_t c) const {
    col_type result{};
    for (auto r = 0; r < rows(); ++r)
      result.value_at(r, 0) = _values[r][c];
    return result;
  }

  auto row(size_t r) const {
    row_type result{};
    for (auto c = 0; c < cols(); ++c)
      result.value_at(0, c) = _values[r][c];
    return result;
  }

private:

  value_type _values[Rows][Cols];

};

template<class Func, class M>
void apply(Func f, const M& m, M& out_m) {
  for (auto r = 0; r < m.rows(); ++r) {
    for (auto c = 0; c < m.cols(); ++c)
      out_m.value_at(r,c) = f(m.value_at(r,c));
  }
}

template<class Func, class M>
auto apply(Func f, const M& m) {
  M result{};
  apply(f, m, result);
  return result;
}

template<class Func, class M>
auto apply(Func f, M&& m) {
  apply(f, m, m);
  return m;
}

template<class Func, class M>
auto& apply(Func f, M& m) {
  apply(f, m, m);
  return m;
}

template<typename T, size_t Rows, size_t N, size_t Cols>
inline auto mul(const matrix<T, Rows, N>& a,
    const matrix<T, N, Cols>& b) {
  matrix<T, Rows, Cols> result{};
  // TODO: consider doing transposition of b, so we don't fall into cache misses.
  for (auto r = 0; r < a.rows(); ++r) {
    for (auto c = 0; c < b.cols(); ++c) {
      auto& val = result.value_at(r, c);
      for (auto i = 0; i < b.rows(); ++i)
        val += a.value_at(r, i) * b.value_at(i, c);
    }
  }

  return result;
}

template<class M>
void normalize(M& m) {
  // Column normalization follows
  typename M::row_type norm{};
  for (auto r = 0; r < m.rows(); ++r) {
    for (auto c = 0; c < m.cols(); ++c) {
      norm.value_at(0, c) += m.value_at(r, c)
        * m.value_at(r, c);
    }
  }

  apply<
    typename M::value_type(typename M::value_type),
    typename M::row_type
  >(std::sqrt, norm);
  for (auto r = 0; r < m.rows(); ++r) {
    for (auto c = 0; c < m.cols(); ++c)
      m.value_at(r, c) /= norm.value_at(0, c);
  }
}

template<class M>
inline M normalized(const M& m) {
  auto result{m};
  normalize(result);
  return result;
}

template<class M>
inline M normalized(M&& m) {
  normalize(m);
  return m;
}

namespace detail_ {

template<typename Op, typename T, size_t Rows, size_t Cols>
inline auto op(const matrix<T, Rows, Cols>& a,
    const matrix<T, Rows, Cols>& b, Op op) {
  matrix<T, Rows, Cols> result{};
  for (auto r = 0; r < a.rows(); ++r) {
    for (auto c = 0; c < b.cols(); ++c) {
      result.value_at(r, c) = op(a.value_at(r, c),
        b.value_at(r, c));
    }
  }

  return result;
}

} // namespace detail_

template<typename T, size_t Rows, size_t Cols>
inline auto operator+(const matrix<T, Rows, Cols>& a,
    const matrix<T, Rows, Cols>& b) {
  return detail_::op(a, b, [](auto u, auto v) { return u+v; });
}

template<typename T, size_t Rows, size_t Cols>
inline auto operator*(const matrix<T, Rows, Cols>& a,
    const matrix<T, Rows, Cols>& b) {
  return detail_::op(a, b, [](auto u, auto v) { return u*v; });
}

} // namespace marslander::linalg

#endif // LINALG_H_
