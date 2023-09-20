#include <string>

namespace marslander {

template<class CharT, class Traits, class Alloc>
static inline std::vector<std::basic_string<CharT, Traits, Alloc>>
split(
  const std::basic_string<CharT, Traits, Alloc>& s,
  const std::basic_string<CharT, Traits, Alloc>& delim,
  bool skip_empty = false
) {

  std::vector<std::basic_string<CharT, Traits, Alloc>> result;
  typename std::basic_string<CharT, Traits, Alloc>::size_type
    sz, pos = 0, end = s.length();

  constexpr auto npos_ = std::basic_string<CharT, Traits, Alloc>::npos;
  while (pos < end) {
    auto fpos = s.find_first_of(delim, pos);
    if (fpos == npos_) fpos = end;
    sz = fpos - pos;
    if (!skip_empty || sz > 0)
      result.push_back(s.substr(pos, sz));
    pos = fpos + 1;
  }

  return result;
}

} // namespace marslander
