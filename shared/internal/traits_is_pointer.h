#include <type_traits>

namespace marslander::traits {

template<typename T>
struct is_pointer : std::false_type {};
template<typename T>
struct is_pointer<T*> : std::true_type {};

template<typename T>
constexpr inline bool is_pointer_v = is_pointer<T>::value;

} // namespace marslander::traits
