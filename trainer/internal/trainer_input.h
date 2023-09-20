#include "global_includes.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <string>

namespace marslander::input {

// algorithm_args defined inside trainer_app.h
static inline bool cvt_alg_args(const std::string& s, trainer::algorithm_args& r) {
  auto tokens = split(s, std::string(";"), true);
  if (tokens.empty()) return false;

  ITERZ_(tokens, tokens);
  std::transform(tokens_it, tokens_end, tokens_it, &trim_copy);

  r.name = std::move(*tokens_it++);
  if (r.name.empty()) return false;

  r.values.resize(tokens.size() - 1);
  auto dst_it = r.values.begin();
  while (!ITERZ_END(tokens)) {
    try { *dst_it++ = std::stod(*tokens_it++); }
    catch (...) { return false; }
  }

  to_lower(r.name);
  return true;
}

} // namespace marslander::input
