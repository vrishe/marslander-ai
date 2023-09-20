#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace marslander {

struct looper {

  static looper& current();
  static looper& main();

  looper();

  using callback_type = std::function<void()>;

  void post(callback_type&& cb);

  template<typename Callback, typename... Args>
  void post(Callback&& cb, Args&&... args) {
    post(std::forward<callback_type>(
      std::bind( std::forward<Callback>(cb),
                 std::forward<Args>(args)... )
    ));
  }

  void run();

private:

  thread_local static looper* _current;
  static looper* volatile _main;

  using buffer_t = std::vector<callback_type>;

  std::array<buffer_t, 2> _buf;
  std::atomic_size_t _c;
  std::condition_variable _cv;
  std::mutex _m;

};

} // namespace marslander
