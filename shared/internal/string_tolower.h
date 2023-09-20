#include <algorithm>
#include <cctype>
#include <string>

namespace marslander {

static inline void to_lower(std::string& s)
{ std::transform(s.begin(), s.end(), s.begin(),
  [](unsigned char c) {return std::tolower(c);}); }

static inline std::string to_lower_copy(std::string s)
{ to_lower(s); return s; }

static inline void to_upper(std::string& s)
{ std::transform(s.begin(), s.end(), s.begin(),
  [](unsigned char c) {return std::toupper(c);}); }

static inline std::string to_upper_copy(std::string s)
{ to_upper(s); return s; }

} // namespace marslander
