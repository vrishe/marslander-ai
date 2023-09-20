#ifndef SHARED_SHARED_H_
#include "internal/string_trim.h"
#endif

#include <algorithm>
#include <cctype>
#include <exception>
#include <string>

namespace marslander::input {

template<typename T, typename FCvt>
void read_input(const std::string& msg, const std::string& msg_err,
    FCvt&& cvt, T& out_result) {
  std::string str;
  for (;;) {
    std::cout << msg << " ";
    std::getline(std::cin, str);
    if (cvt(str, out_result)) {
      break;
    }

    std::cerr << msg_err << std::endl << std::endl;
  }
}

namespace answers {

static inline bool yes_n(const std::string& r) { return r == "y" || r == "yes"; }
static inline bool yes_y(const std::string& r) { return r.empty() || r == "y" || r == "yes"; }

} // namespace answers

template<class Pred>
bool wait_answer(Pred&& p) {
  std::string reply; getline(std::cin, reply);
  std::transform(reply.begin(), reply.end(), reply.begin(),
    [](unsigned char c){ return std::tolower(c); });
  return p(reply);
}

static inline bool wait_answer() {
  return wait_answer(answers::yes_n);
}

namespace detail_ {

static inline unsigned long stoul(const std::string& z) {
  if (z.size() > 0 && z.front() != '-') return std::stoul(z);
  throw std::invalid_argument(z);
}

template<typename T, typename CvtF>
bool cvt_num(const std::string& s, T& r,
    T min, T max, T def, CvtF&& cvt) {
  if (s.empty()) return (r = std::clamp(def, min, max), true);
  try { 
    r = std::move(cvt(ltrim_copy(s)));
    return min <= r && r <= max;
  }
  catch (...) { return false; }
}

} // namespace detail_

static inline bool cvt_num_dbl(const std::string& s, double& r,
    double min, double max, double def) {
  using namespace std::placeholders;
  return detail_::cvt_num(s, r, min, max, def,
    [](const auto& z) { return std::stod(z); });
}

static inline bool cvt_num_ul(const std::string& s, unsigned long& r,
    unsigned long min, unsigned long max, unsigned long def) {
  return detail_::cvt_num(s, r, min, max, def, detail_::stoul);
}

enum class units {
  amount,
  percentage,
  percent
};

struct unit_value final {

  units unit;
  union {
    double d;
    size_t i;
  } value;

  static unit_value amount(size_t amount) {
    unit_value r;
    r.unit = units::amount;
    r.value.i = amount;
    return r;
  }

  static unit_value percentage(double percentage) {
    unit_value r;
    r.unit = units::percentage;
    r.value.d = std::clamp(percentage, .0, 1.);
    return r;
  }

  static unit_value percent(double percent) {
    unit_value r;
    r.unit = units::percent;
    r.value.d = percent >= 0 ? percent : 0;
    return r;
  }

  size_t operator()(size_t total) const {
    size_t result;

    switch (unit) {
      case units::percent:
        result = static_cast<size_t>(std::round(total / 100. * value.d));
        break;
      case units::percentage:
        result = static_cast<size_t>(std::round(total * value.d));
        break;
      case units::amount:
        result = value.i;
        break;

      default:
        return size_t{};
    }

    return std::min(result, total);
  }

};

static inline bool cvt_unit(const std::string& s, unit_value& r,
    size_t max, size_t def) {

  if (s.empty()) {
    r.unit = units::amount;
    r.value.i = std::clamp(def, size_t{}, max);
    return true;
  }

  auto st = std::move(ltrim_copy(s));
  if (st.back() == '%') {
    r.unit = units::percent;
    try { r.value.d = std::stod(st); }
    catch (...) { return false; }
    return .0 <= r.value.d && r.value.d <= 100.;
  }

  r.unit = units::percentage;
  try { r.value.d = std::stod(st); }
  catch (...) { return false; }

  if (r.value.d >= 1) {
    r.unit = units::amount;
    try { r.value.i = detail_::stoul(s); }
    catch (...) { return false; }
    return r.value.i <= max;
  }

  return 0 <= r.value.d && r.value.d < 1;
}

} // namespace marslander::input
