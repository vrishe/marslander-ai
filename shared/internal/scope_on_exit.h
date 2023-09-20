#include <functional>

namespace marslander::scope {
  
struct on_exit {
  template<typename F>
  on_exit(F&& func)
    : _f(func) {}
  // on_exit(on_exit&& other)
  //   : _f(exchange(other._f, []{})) {}
  ~on_exit() { _f(); }
private:
  std::function<void()> _f;
};

} // namespace marslander
