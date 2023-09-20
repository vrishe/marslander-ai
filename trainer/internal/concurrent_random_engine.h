#include <algorithm>
#include <atomic>
#include <functional>
#include <list>
#include <mutex>
#include <random>
#include <thread>

namespace marslander::trainer {

template<class Engine, size_t ThreadBufSz = 64>
class concurrent_random_engine final {

public:

  using result_type = typename Engine::result_type;

  static constexpr size_t buffer_size = ThreadBufSz;
  static constexpr result_type min() { return Engine::min(); }
  static constexpr result_type max() { return Engine::max(); }

private:

  struct thread_buffer_t;
  struct tls_entry_t {
    std::unique_ptr<thread_buffer_t> *p;
    ~tls_entry_t() {
      if (!p || !*p) return;
      (*p)->reset_owner();
      (*p).reset();
    }
  };

  using tls_entries_list_t = std::list<tls_entry_t>;
  tls_entries_list_t _entries;
  std::mutex _mtx_entries;

  class thread_buffer_t final {
    std::atomic<concurrent_random_engine *> _owner;
    typename tls_entries_list_t::iterator _it;

    result_type _data[buffer_size];
    result_type * const _data_end, *_data_front;

  public:

    using iterator = result_type *;
    using const_reference = const result_type &;

    size_t capacity() const noexcept { return buffer_size; }
    bool empty() const noexcept { return _data_end <= _data_front; }
    const_reference pop_front() noexcept { return *_data_front++; }
    iterator rewind() noexcept { return _data_front = _data; }

    void reset_owner() { _owner.store(nullptr); }
    ~thread_buffer_t() {
      auto owner = _owner.load();
      if (!owner) return;
      _it->p = nullptr;
      const std::lock_guard l{owner->_mtx_entries};
      owner->_entries.erase(_it);
    }
    thread_buffer_t(concurrent_random_engine *owner,
        typename tls_entries_list_t::iterator it)
      : _owner{owner},
        _it{it},
        _data_end{_data + buffer_size},
        _data_front{_data_end}
      {}
  };

  thread_buffer_t& thread_buffer() {
    static thread_local std::unique_ptr<thread_buffer_t> my_buffer;
    if (!my_buffer) {
      typename tls_entries_list_t::iterator it;
      {
        const std::lock_guard l{_mtx_entries};
        it = _entries.insert(_entries.cend(), tls_entry_t{});
      }
      my_buffer = std::move(std::make_unique<thread_buffer_t>(this, it));
      it->p = &my_buffer;
    }
    return *my_buffer;
  }

  Engine _base;
  std::mutex _mtx_base;

public:

  concurrent_random_engine(Engine&& prng)
    : _base{std::move(prng)}
    {}

  ~concurrent_random_engine() {
    const std::lock_guard l{_mtx_entries};
    _entries.clear();
  }

  const Engine& base() const noexcept { return _base; }

  result_type operator()() {
    auto& buf = thread_buffer();
    if (!buf.empty()) return buf.pop_front();

    const std::lock_guard l{_mtx_base};
    auto res = _base();
    std::generate_n(buf.rewind(), buf.capacity(), std::ref(_base));
    return res;
  }

};

} // namespace marslander::trainer
