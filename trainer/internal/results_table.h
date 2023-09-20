#include <algorithm>
#include <cstddef>
#include <tuple>
#include <utility>

namespace marslander::trainer {

template<typename TPtr>
class results_table_iterator final {

  const TPtr _base;
  const ptrdiff_t _stride;

  size_t _row;

  static bool check_compatible(const results_table_iterator& a,
      const results_table_iterator& b) {
    return a._base == b._base && a._stride == b._stride;
  }

public:

  using difference_type = size_t;
  using value_type =  std::tuple<size_t, TPtr, TPtr>;
  using pointer = void;
  using reference = value_type&&;
  using iterator_category = std::random_access_iterator_tag;

  results_table_iterator(TPtr base, size_t r, ptrdiff_t stride)
    : _base(base), _row(r), _stride(stride)
    {}

  results_table_iterator(const results_table_iterator& ) = default;
  results_table_iterator(      results_table_iterator&&) = default;

  bool operator==(const results_table_iterator& other) const {
    return _base == other._base && _row == other._row; }
  bool operator!=(const results_table_iterator& other) const {
    return !(*this == other); }

  results_table_iterator& operator++()    { return (++_row, *this); }
  results_table_iterator  operator++(int) {
    return results_table_iterator(_base, _row++, _stride); }
  results_table_iterator& operator--()    { return (--_row, *this); }
  results_table_iterator  operator--(int) {
    return results_table_iterator(_base, _row--, _stride); }

  results_table_iterator& operator+=(difference_type n) {
    return (_row += n, *this); }
  results_table_iterator& operator-=(difference_type n) {
    return (_row -= n, *this); }

  value_type operator*() const { return this->operator[](0); }
  value_type operator[](difference_type n) const {
    auto start_ptr = _base + (_row + n) * _stride;
    return std::make_tuple(_row, start_ptr, start_ptr + _stride);
  }

  static difference_type diff(const results_table_iterator& a,
      const results_table_iterator& b) {
    assert(check_compatible(a, b));
    return a._row - b._row;
  }

  static bool less(const results_table_iterator& a,
      const results_table_iterator& b) {
    assert(check_compatible(a, b));
    return a._row < b._row;
  }
};

#define RT_IT_TMPL_ results_table_iterator<TPtr>

template<typename TPtr>
RT_IT_TMPL_ operator+(RT_IT_TMPL_ i, typename RT_IT_TMPL_::difference_type n)
{ return i += n; }
template<typename TPtr>
RT_IT_TMPL_ operator+(typename RT_IT_TMPL_::difference_type n, RT_IT_TMPL_ i)
{ return i += n; }
template<typename TPtr>
RT_IT_TMPL_ operator-(RT_IT_TMPL_ i, typename RT_IT_TMPL_::difference_type n)
{ return i -= n; }

template<typename TPtr>
typename RT_IT_TMPL_::difference_type operator-(
  const RT_IT_TMPL_& a, const RT_IT_TMPL_& b)
{ return a.diff(a, b); }

template<typename TPtr>
bool operator<(const RT_IT_TMPL_& a, const RT_IT_TMPL_& b)
{ return a.less(a, b); }
template<typename TPtr>
bool operator>(const RT_IT_TMPL_& a, const RT_IT_TMPL_& b)
{ return b < a; }
template<typename TPtr>
bool operator<=(const RT_IT_TMPL_& a, const RT_IT_TMPL_& b)
{ return !(a > b); }
template<typename TPtr>
bool operator>=(const RT_IT_TMPL_& a, const RT_IT_TMPL_& b)
{ return !(a < b); }

#undef RT_IT_TMPL_

template<typename T>
class results_table_tmpl_ final {

public:

  using value_type = T;

private:

  using       data_ptr_t =       value_type*;
  using const_data_ptr_t = const value_type*;

  size_t _c, _r;
  data_ptr_t _data;

public:

  results_table_tmpl_()
    : _c{}, _r{}, _data{}
    {}

  results_table_tmpl_(size_t cols, size_t rows) {
    resize(cols, rows);
  }

  results_table_tmpl_(results_table_tmpl_&& other) {
    _c = other._c;
    _r = other._r;
    _data = std::exchange(other._data, nullptr);
  }

  ~results_table_tmpl_() {
    delete[] _data;
  }

  results_table_tmpl_& operator=(results_table_tmpl_&& other) {
    delete[] _data;

    _c = other._c;
    _r = other._r;
    _data = std::exchange(other._data, nullptr);

    return *this;
  }

  size_t cols() const { return _c; }
  size_t rows() const { return _c; }

  using       iterator = results_table_iterator<      data_ptr_t>;
  using const_iterator = results_table_iterator<const_data_ptr_t>;

  auto  begin()       { return       iterator(_data, 0, _c); }
  auto cbegin() const { return const_iterator(_data, 0, _c); }

  auto  end()       { return       iterator(_data, _r, _c); }
  auto cend() const { return const_iterator(_data, _r, _c); }

  auto row_at(size_t r)       { return r < _r ?       iterator(_data, r, _c) :  end(); }
  auto row_at(size_t r) const { return r < _r ? const_iterator(_data, r, _c) : cend(); }

        data_ptr_t operator[](size_t r)       { return &_data[_c * r]; }
  const_data_ptr_t operator[](size_t r) const { return &_data[_c * r]; }

  void resize(size_t cols, size_t rows, T fill_val = T{}) {
    auto sz_new = cols * rows;
    auto sz = _c * _r;

    _c = cols;
    _r = rows;

    if (sz != sz_new) {
      delete[] _data;
      _data = new value_type[sz = sz_new];
    }

    std::fill(_data, _data + sz, fill_val);
  }

};

using results_table = results_table_tmpl_<double>;

} // namespace marslander::trainer
