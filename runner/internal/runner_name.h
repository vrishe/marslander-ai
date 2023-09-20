#if defined(__linux__)

#include <cstring>
#include <sstream>

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <limits.h>
#include <unistd.h>

namespace marslander::runner {

static inline std::string name() {
  std::string hname(HOST_NAME_MAX + 1, '\0');
  gethostname(hname.data(), HOST_NAME_MAX);
  hname.resize(strlen(hname.c_str()));
  return (std::stringstream() << hname << "_" << getpid()).str();
}

} // namespace marslander::runner

#endif // defined(__linux__)
