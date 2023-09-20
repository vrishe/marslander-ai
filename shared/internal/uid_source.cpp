#include "uid_source.h"

#include <limits>

namespace marslander {

uid_source::uid_source() : uid_{} {};

uid_source::uid_source(uid_t v0) : uid_{v0} {}

uid_source::uid_source(uid_source&& other)
    : uid_{other.uid_.exchange(std::numeric_limits<uid_t>::max())}
  {}

uid_source& uid_source::operator=(uid_source&& other) {
  uid_ = other.uid_.exchange(std::numeric_limits<uid_t>::max());
  return *this;
}

uid_t uid_source::next_uid() { return ++uid_; }

} // namespace marslander
