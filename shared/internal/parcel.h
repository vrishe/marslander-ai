#include <stdexcept>
#include <type_traits>
#include <utility>

namespace marslander {

namespace detail_ {

  template<typename T, bool = std::is_copy_constructible_v<T>>
  struct can_copy_s_;

  template<typename T>
  struct can_copy_s_<T, true> {
    static const T& check(const T& v) { return v; }
  };

  template<typename T>
  struct can_copy_s_<T, false> {
    static T&& check(const T& v) {
      throw std::logic_error("Can't copy non-copyable value parcel.");
    }
  };

} // namespace detail_

template<typename T>
struct parcel {

  using value_type = std::remove_reference_t<T>;

  parcel() = delete;

  parcel(value_type& value)
    : val_{value} { }

  parcel(value_type&& value)
    : val_{std::forward<value_type>(value)} { }

  parcel(const parcel& other)
    : val_{detail_::can_copy_s_<value_type>::check(other.val_)} { }

  parcel(parcel&& other)
    : val_{std::forward<value_type>(other.val_)} { }

  value_type&& unwrap() { return std::move(val_); }

  const value_type& value() const { return val_; }

private:

  value_type val_;

};

template<typename T>
auto make_parcel(T&& value) {
  return parcel<T>(std::forward<T>(value));
}

} // namespace marslander
