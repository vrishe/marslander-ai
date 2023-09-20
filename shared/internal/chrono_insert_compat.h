#if (__cplusplus < 202002L)

#include <chrono>
#include <ratio>
#include <sstream>

namespace std::chrono {

namespace detail_ {

template<typename Period>
constexpr inline const char* units_suffix = "s";
template<> constexpr inline const char* units_suffix<std::atto> = "as";
template<> constexpr inline const char* units_suffix<std::femto> = "fs";
template<> constexpr inline const char* units_suffix<std::pico> = "ps";
template<> constexpr inline const char* units_suffix<std::nano> = "ns";
template<> constexpr inline const char* units_suffix<std::micro> = "µs";
template<> constexpr inline const char* units_suffix<std::milli> = "ms";
template<> constexpr inline const char* units_suffix<std::centi> = "cs";
template<> constexpr inline const char* units_suffix<std::deci> = "ds";
template<> constexpr inline const char* units_suffix<std::deca> = "das";
template<> constexpr inline const char* units_suffix<std::hecto> = "hs";
template<> constexpr inline const char* units_suffix<std::kilo> = "ks";
template<> constexpr inline const char* units_suffix<std::mega> = "Ms";
template<> constexpr inline const char* units_suffix<std::giga> = "Gs";
template<> constexpr inline const char* units_suffix<std::tera> = "Ts";
template<> constexpr inline const char* units_suffix<std::peta> = "Ps";
template<> constexpr inline const char* units_suffix<std::exa> = "Es";
template<> constexpr inline const char* units_suffix<std::ratio<60>> = "min";
template<> constexpr inline const char* units_suffix<std::ratio<3600>> = "h";
template<> constexpr inline const char* units_suffix<std::ratio<86400>> = "d";

} // namespace detail_

template<
    class CharT,
    class Traits,
    class Rep,
    class Period
> std::basic_ostream<CharT, Traits>&
operator<<( std::basic_ostream<CharT, Traits>& os,
            const std::chrono::duration<Rep, Period>& d ) {
  std::basic_ostringstream<CharT, Traits> s;
  s.flags(os.flags());
  s.imbue(os.getloc());
  s.precision(os.precision());
  s << d.count() << detail_::units_suffix<Period>;
  return os << s.str(); 
}

} // namespace std::chrono

#endif
