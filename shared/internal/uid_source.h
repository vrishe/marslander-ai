#include <atomic>
#include <cstdint>

namespace marslander {

using uid_t = uint64_t;

inline constexpr uid_t no_id = uid_t(0);

struct uid_source {

  uid_source();

  uid_source(uid_t);

  uid_source(uid_source&&);

  uid_source& operator=(      uid_source&&);

  uid_t next_uid();

  uid_t value() const noexcept { return uid_; }
  operator uid_t() const noexcept { return value(); }

private:

  std::atomic<uid_t> uid_;

};

} // namespace marslander
