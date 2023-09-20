#include <type_traits>
#include <utility>

namespace marslander {

template<typename T, typename M>
struct this_binding_s_ {

  typedef M T::* mptr_;
  typedef   T  * thiz_;

  thiz_ p_;
  mptr_ m_;

  template<typename... Args>
  void operator()(Args&&... args) noexcept(noexcept(std::declval<M>())) {
    std::invoke(m_, p_, std::forward<Args>(args)...);
  }

};

template<typename T, typename M>
auto this_binding(T* target, M T::* mem_fn) noexcept
    -> std::enable_if_t< std::is_invocable_v<M>, this_binding_s_<T, M> > {
  return { target, mem_fn };
}

} // namespace marslander
