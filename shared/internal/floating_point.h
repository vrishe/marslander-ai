#include <cassert>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <cfenv>

namespace marslander::fp {

namespace detail_ {

template<typename T>
struct is_double_t : std::false_type {};
template<>
struct is_double_t<double> : std::true_type {};

template<typename T>
constexpr inline bool is_iec559_double
  = is_double_t<T>::value && std::numeric_limits<T>::is_iec559;

#define ASSERT_IEC559_DOUBLE(type) \
static_assert(is_iec559_double<type>, "IEEE754-compliant double required.")

struct _iec559_double {
  unsigned mantissa_l : 32;
  unsigned mantissa_h : 20;
  unsigned exponent   : 11;
  unsigned sign       : 1;
};

template<typename T>
_iec559_double* double_cast(T& var) {
  return reinterpret_cast<_iec559_double*>(&var); }
template<typename T>
const _iec559_double* double_cast(const T& var) {
  return reinterpret_cast<const _iec559_double*>(&var); }

struct fenv_scope final {
  static constexpr uint16_t mode = 0x27f;
  std::fenv_t _fenv;
  fenv_scope() {
    fegetenv(&_fenv);
    asm ("fldcw %0" : : "m" (mode) );
  }
  ~fenv_scope() {
    fesetenv(&_fenv);
  }
};

#define SUM_COMPENSATION_ \
( double_cast(a)->exponent < double_cast(b)->exponent )\
    ? (b - t) + a : (a - t) + b;

static inline double add_two(double a, double b, double& b_dst) {
  auto t = a + b;
  b_dst = SUM_COMPENSATION_;
  return t;
}

static inline double add_two_alt(double a, double b, double& a_dst) {
  auto t = a + b;
  a_dst = t;
  return SUM_COMPENSATION_;
}

#undef SUM_COMPENSATION_

constexpr inline double ZERO {};

static inline void add_two(double& a, double& b) { a = add_two(a, b, b); }
static inline bool check_rounding(double s, double e) {
  return s != ZERO && s * e > ZERO
    && double_cast(s)->mantissa_l == 0
    && double_cast(s)->mantissa_h == 0;
}

template<class ValuesIt>
double fast_sum_r_(ValuesIt start, ValuesIt end, bool r_c = false) {
  double e1, e2, e_m, half_ulp, s, s1, s2, s_t;
  double ev_d = ZERO;

  auto sum_it = start;
  s = std::exchange(*sum_it++, ZERO);
  if (unlikely_(sum_it == end)) return s;
  do { add_two(s, *sum_it++); } while (sum_it != end);
  if (unlikely_(s != s)) return s;

  for(;;) {
    sum_it = start;
    s_t = std::exchange(*sum_it++, ZERO);

    int count = 0;
    unsigned max = 0;
    for (auto run_it = sum_it; run_it != end; run_it++) {
      s_t = add_two(s_t, *run_it, *sum_it);
      if (*sum_it != ZERO) {
        sum_it++;
        ++count;
        auto s_t_exponent = double_cast(s_t)->exponent;
        if (max < s_t_exponent) max = s_t_exponent;
      }
    }

    // Didn't we fall outside the given bounds?
    // This is barely possible, as the first element always 
    // becomes zero due to summation, thus leading the resulting
    // iterator to stay at least one step before the end.
    assert(sum_it != end);

    constexpr double EPS  {std::numeric_limits<double>::epsilon() / 2};

    if (max > 0) {
      double_cast(ev_d)->exponent = max;
      ev_d *= EPS;
      e_m = ev_d * count;
    }
    else
      e_m = ZERO;

    add_two(s, s_t);
    *sum_it++ = s_t;
    end = sum_it;

    auto s_exponent = double_cast(s)->exponent;
    if (s_exponent > 0) {
      double_cast(half_ulp)->exponent = s_exponent;
      half_ulp *= EPS;
    }
    else
      half_ulp = ZERO;

    if (e_m != ZERO && e_m >= half_ulp) continue;
    if (r_c) return s;

    s1 = s2 = s_t;
    e1 =  e_m;
    e2 = -e_m;
    add_two(s1, e1);
    add_two(s2, e2);
    if (s + s1 != s || s + s2 != s
        || check_rounding(s1, e1) || check_rounding(s2, e2)) {
      s1 = fast_sum_r_(start, end, true);
      add_two(s, s1);
      s2 = fast_sum_r_(start, end, true);
      if (check_rounding(s1, s2)) {
        double_cast(s1)->mantissa_l |= 1;
        s += s1;
      }
    }
    return s;
  }
}

template<class ValuesIt>
double fast_sum(ValuesIt start, ValuesIt end, std::forward_iterator_tag,
    bool r_c = false) {
  ASSERT_IEC559_DOUBLE(typename std::iterator_traits<ValuesIt>::value_type);
  if (start == end) return ZERO;
  [[maybe_unused]] fenv_scope sentry_;
  return fast_sum_r_(start, end);
}

class accumulator_ctx {

  constexpr static size_t HALF_MANTISSA {26};
  constexpr static size_t MAX_N         {1 << HALF_MANTISSA};
  constexpr static size_t N_EXPONENT    {2048};
  constexpr static size_t N2_EXPONENT   {N_EXPONENT << 1};

  typedef double* value_ptr;

  size_t c_num;
  value_ptr t_s, t_s2;

protected:

  accumulator_ctx()
    : t_s {new double[N2_EXPONENT]},
      t_s2{new double[N2_EXPONENT]}
    { reset(); }

public:

  virtual ~accumulator_ctx() {
    delete[] t_s;
    delete[] t_s2;
  }

  template<typename Value>
  void add_one(Value x) {
    ASSERT_IEC559_DOUBLE(Value);
    [[maybe_unused]] fenv_scope sentry_;
    unsigned exp;
    if (c_num >= MAX_N) {
      std::fill(t_s2, t_s2 + N2_EXPONENT, ZERO);
      for (size_t i = 0; i < N2_EXPONENT; i++) {
        exp = double_cast(t_s[i])->exponent;
        t_s2[exp+N_EXPONENT] += add_two_alt(t_s2[exp], t_s[i], t_s2[exp]);
      }
      std::swap(t_s, t_s2);
      c_num = N2_EXPONENT; // t_s has added N2_EXPONENT numbers
    }
    exp = double_cast(x)->exponent;
    t_s[exp+N_EXPONENT] += add_two_alt(t_s[exp], x, t_s[exp]);
    c_num++;
  }

  template<class ValuesIt>
  void add_many(ValuesIt start, ValuesIt end, std::input_iterator_tag) {
    ASSERT_IEC559_DOUBLE(typename std::iterator_traits<ValuesIt>::value_type);
    if (start == end) return;
    [[maybe_unused]] fenv_scope sentry_;
    unsigned exp;
    while (start != end) {
      auto x = *start++;
      exp = double_cast(x)->exponent;
      t_s[exp+N_EXPONENT] += add_two_alt(t_s[exp], x, t_s[exp]);
      if (unlikely_(++c_num >= MAX_N)) {
        std::fill(t_s2, t_s2 + N2_EXPONENT, ZERO);
        for (size_t i = 0; i < N2_EXPONENT; i++) {
          exp = double_cast(t_s[i])->exponent;
          t_s2[exp+N_EXPONENT] += add_two_alt(t_s2[exp], t_s[i], t_s2[exp]);
        }
        std::swap(t_s, t_s2);
        c_num = N2_EXPONENT; // t_s has added N2_EXPONENT numbers
      }
    }
  }

  void reset() {
    c_num = 0;
    std::fill(t_s, t_s + N2_EXPONENT, ZERO);
  }

  double result() {
    auto t_s2_end = std::copy_if(t_s, t_s+N2_EXPONENT, t_s2,
      [](auto v) { return v != ZERO; });

    reset();

    if (t_s2 == t_s2_end) return ZERO;
    [[maybe_unused]] fenv_scope sentry_;
    return fast_sum_r_(t_s2, t_s2_end);
  }
};

} // namespace detail_

template<class ValuesIt>
double fast_sum(ValuesIt start, ValuesIt end) { 
  return detail_::fast_sum(start, end,
    typename std::iterator_traits<ValuesIt>::iterator_category());
}

class accumulator final : detail_::accumulator_ctx {

public:

  void add(double x) { add_one(x); }

  template<class ValuesIt>
  void add(ValuesIt start, ValuesIt end) {
    add_many(start, end,
      typename std::iterator_traits<ValuesIt>::iterator_category());
  }

  using detail_::accumulator_ctx::reset;
  using detail_::accumulator_ctx::result;

};

} // namespace marslander::fp
