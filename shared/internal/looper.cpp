#include "looper.h"
#include "internal/scope_on_exit.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace marslander {

using namespace std;

thread_local looper* looper::_current = nullptr;
looper* volatile looper::_main = nullptr;

looper& looper::current() {

  if (!_current) {
    try { return main(); }
    catch (runtime_error&) {
      stringstream out;
      out << "Can't get a looper for Thread #" << this_thread::get_id();
      throw runtime_error(out.str());
    }
  }

  return *_current;
}

looper& looper::main() {
  auto m = _main;
  if (m) return *m;
  throw runtime_error("There is no main looper.");
}

namespace {
constexpr inline size_t buf_sz = 64;
} // namespace 

looper::looper() : _c{} {
  if (_main == nullptr) _main = this;

  _buf.front().reserve(buf_sz);
}

void looper::post(function<void()>&& cb) {
  ++_c;
  [[maybe_unused]] scope::on_exit c_sentry_([&c=_c]{--c;});

  lock_guard lk { _m };
  auto& q = _buf.front();
  q.push_back(move(cb));
  if (_c == 1 || q.size() >= buf_sz)
    _cv.notify_one();
}

void looper::run() {
  _current = this;
  auto invoke_callback_ = [](buffer_t::reference cb)
    noexcept { cb(); };

  _buf.back().reserve(buf_sz);
  auto wait_pred = [&buf = _buf]{ return buf.front().size() > 0; };

  for(;;) {

    {
      unique_lock lk { _m };
      _cv.wait(lk, wait_pred);
      swap(_buf.front(), _buf.back());
    }

    auto& bbuf = _buf.back();
    for (auto& cb : bbuf)
      invoke_callback_(cb);
    bbuf.clear();
  }
}

} // namespace marslander
