#include <future>

#define value_type_of(expr) decltype(NS_value_type_of_::value_type(expr))

namespace NS_value_type_of_ {

template<typename T>
T value_type(const std::future<T>&);
template<typename T>
T value_type(std::future<T>*);
template<typename T>
T value_type(std::future<T>* const);
template<typename T>
T value_type(std::future<T>* volatile);
template<typename T>
T value_type(std::future<T>* const volatile);

template<typename T>
typename std::remove_pointer_t<
    std::remove_reference_t<T>
  >::value_type
value_type(T);

} // namespace NS_value_type_of_
