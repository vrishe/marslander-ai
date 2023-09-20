#include "base64.h"

namespace marslander::base64 {

namespace {

inline uint64_t flip_byteorder(uint64_t x) {
  constexpr uint64_t m8 = 0x00FF00FF00FF00FF;
  x = ((x & m8) << 8) | (x >> 8) & m8;
  constexpr uint64_t m16 = 0x0000FFFF0000FFFF;
  x = ((x & m16) << 16) | (x >> 16) & m16;
  return (x << 32) | (x >> 32);
}

inline uint64_t from_char(char c);
inline char to_char(uint64_t v);

} // namespace

size_t decode(const char* src, size_t src_sz,
    unsigned char* dst, size_t dst_sz) {

  if (src == nullptr) return src_sz;

  src_sz &= ~3;
  auto tail_sz = (src[src_sz - 2] == '=')
    + (src[src_sz - 1] == '=');

  auto req_sz = (src_sz / 4) * 3 - tail_sz;
  if (dst == nullptr || req_sz > dst_sz) return req_sz;

  auto n = req_sz / 3;
  tail_sz = 4 - tail_sz;

  uint64_t v;
  for (; n >= 2; n -= 2, src += 8, dst += 6) {
    v = flip_byteorder(
        (from_char(* src     ) << 58)
      | (from_char(*(src + 1)) << 52)
      | (from_char(*(src + 2)) << 46)
      | (from_char(*(src + 3)) << 40)
      | (from_char(*(src + 4)) << 34)
      | (from_char(*(src + 5)) << 28)
      | (from_char(*(src + 6)) << 22)
      | (from_char(*(src + 7)) << 16));

    *(dst    ) = (v      ) & 0xFF;
    *(dst + 1) = (v >>  8) & 0xFF;
    *(dst + 2) = (v >> 16) & 0xFF;
    *(dst + 3) = (v >> 24) & 0xFF;
    *(dst + 4) = (v >> 32) & 0xFF;
    *(dst + 5) = (v >> 40) & 0xFF;
  }

  int offset = 64;
  if (n) {
    v = (from_char(* src     ) << 58)
      | (from_char(*(src + 1)) << 52)
      | (from_char(*(src + 2)) << 46)
      | (from_char(*(src + 3)) << 40);

    offset = 40;
    src += 4;
  }
  else v = 0;

  switch (tail_sz) {
    case 3: v |= from_char(*src++) << (offset -= 6);
    case 2: v |= from_char(*src++) << (offset -= 6);
            v |= from_char(*src  ) << (offset -= 6);
  }

  v = flip_byteorder(v);

  switch ((size_t(64 + 7 - offset) & ~7) >> 3) {
    case 6: *dst++ = v & 0xFF; v >>= 8;
    case 5: *dst++ = v & 0xFF; v >>= 8;
    case 4: *dst++ = v & 0xFF; v >>= 8;
    case 3: *dst++ = v & 0xFF; v >>= 8;
    case 2: *dst++ = v & 0xFF; v >>= 8;
    case 1: *dst   = v & 0xFF;
  }

  return 0;
}

size_t encode(const unsigned char* src, size_t src_sz,
    char* dst, size_t dst_sz) {

  if (src == nullptr) return src_sz;

  auto req_sz = ((src_sz + 2) / 3) * 4;
  if (dst == nullptr || req_sz > dst_sz) return req_sz;

  auto n = src_sz / 3;
  const auto tail_sz = src_sz - (n * 3);

  uint64_t v;
  for (; n >= 2; n -= 2, src += 6, dst += 8) {
    v = flip_byteorder(*reinterpret_cast<const uint64_t*>(src));
    v = ((v & 0x000000FFFFFFFFFF) >> 8) | v & 0xFFFFFF0000000000;
    v = flip_byteorder(
        (v >> 2) & 0x3F0000003F000000
      | (v >> 4) & 0x003F0000003F0000
      | (v >> 6) & 0x00003F0000003F00
      | (v >> 8) & 0x0000003F0000003F);

    *(dst    ) = to_char((v      ) & 0xFF);
    *(dst + 1) = to_char((v >>  8) & 0xFF);
    *(dst + 2) = to_char((v >> 16) & 0xFF);
    *(dst + 3) = to_char((v >> 24) & 0xFF);
    *(dst + 4) = to_char((v >> 32) & 0xFF);
    *(dst + 5) = to_char((v >> 40) & 0xFF);
    *(dst + 6) = to_char((v >> 48) & 0xFF);
    *(dst + 7) = to_char((v >> 56) & 0xFF);
  }

  v = 0;
  {
    auto offset = 64;
    // bytes are being read out in reverse order, no additional flip is needed.
    switch (3 * n + tail_sz) {
      case 5: v |= uint64_t(*src++) << (offset -= 8);
      case 4: v |= uint64_t(*src++) << (offset -= 8);
      case 3: v |= uint64_t(*src++) << (offset -= 8);
      case 2: v |= uint64_t(*src++) << (offset -= 8);
      case 1: v |= uint64_t(*src  ) << (offset -= 8);
    }
  }
  v = ((v & 0x000000FFFFFFFFFF) >> 8) | v & 0xFFFFFF0000000000;
  v = flip_byteorder(
      (v >> 2) & 0x3F0000003F000000
    | (v >> 4) & 0x003F0000003F0000
    | (v >> 6) & 0x00003F0000003F00
    | (v >> 8) & 0x0000003F0000003F);

  if (n) {
    *(dst    ) = to_char((v      ) & 0xFF);
    *(dst + 1) = to_char((v >>  8) & 0xFF);
    *(dst + 2) = to_char((v >> 16) & 0xFF);
    *(dst + 3) = to_char((v >> 24) & 0xFF);
    dst += 4;
    v >>= 32;
  }

  if (tail_sz) {
    *(dst    ) = to_char((v      ) & 0xFF);
    *(dst + 1) = to_char((v >>  8) & 0xFF);
    *(dst + 2) = tail_sz > 1
      ? to_char((v >> 16) & 0xFF)
      : '=';
    *(dst + 3) = '=';
  }

  return 0;
}

namespace {

uint64_t from_char(char c) {
  switch (c) {
    case 'A': return  0;
    case 'B': return  1;
    case 'C': return  2;
    case 'D': return  3;
    case 'E': return  4;
    case 'F': return  5;
    case 'G': return  6;
    case 'H': return  7;
    case 'I': return  8;
    case 'J': return  9;
    case 'K': return 10;
    case 'L': return 11;
    case 'M': return 12;
    case 'N': return 13;
    case 'O': return 14;
    case 'P': return 15;
    case 'Q': return 16;
    case 'R': return 17;
    case 'S': return 18;
    case 'T': return 19;
    case 'U': return 20;
    case 'V': return 21;
    case 'W': return 22;
    case 'X': return 23;
    case 'Y': return 24;
    case 'Z': return 25;
    case 'a': return 26;
    case 'b': return 27;
    case 'c': return 28;
    case 'd': return 29;
    case 'e': return 30;
    case 'f': return 31;
    case 'g': return 32;
    case 'h': return 33;
    case 'i': return 34;
    case 'j': return 35;
    case 'k': return 36;
    case 'l': return 37;
    case 'm': return 38;
    case 'n': return 39;
    case 'o': return 40;
    case 'p': return 41;
    case 'q': return 42;
    case 'r': return 43;
    case 's': return 44;
    case 't': return 45;
    case 'u': return 46;
    case 'v': return 47;
    case 'w': return 48;
    case 'x': return 49;
    case 'y': return 50;
    case 'z': return 51;
    case '0': return 52;
    case '1': return 53;
    case '2': return 54;
    case '3': return 55;
    case '4': return 56;
    case '5': return 57;
    case '6': return 58;
    case '7': return 59;
    case '8': return 60;
    case '9': return 61;
    case '+': return 62;
    case '/': return 63;
    default : return 255;
  }
}

char to_char(uint64_t v) {
  switch (v) {
    case  0: return 'A';
    case  1: return 'B';
    case  2: return 'C';
    case  3: return 'D';
    case  4: return 'E';
    case  5: return 'F';
    case  6: return 'G';
    case  7: return 'H';
    case  8: return 'I';
    case  9: return 'J';
    case 10: return 'K';
    case 11: return 'L';
    case 12: return 'M';
    case 13: return 'N';
    case 14: return 'O';
    case 15: return 'P';
    case 16: return 'Q';
    case 17: return 'R';
    case 18: return 'S';
    case 19: return 'T';
    case 20: return 'U';
    case 21: return 'V';
    case 22: return 'W';
    case 23: return 'X';
    case 24: return 'Y';
    case 25: return 'Z';
    case 26: return 'a';
    case 27: return 'b';
    case 28: return 'c';
    case 29: return 'd';
    case 30: return 'e';
    case 31: return 'f';
    case 32: return 'g';
    case 33: return 'h';
    case 34: return 'i';
    case 35: return 'j';
    case 36: return 'k';
    case 37: return 'l';
    case 38: return 'm';
    case 39: return 'n';
    case 40: return 'o';
    case 41: return 'p';
    case 42: return 'q';
    case 43: return 'r';
    case 44: return 's';
    case 45: return 't';
    case 46: return 'u';
    case 47: return 'v';
    case 48: return 'w';
    case 49: return 'x';
    case 50: return 'y';
    case 51: return 'z';
    case 52: return '0';
    case 53: return '1';
    case 54: return '2';
    case 55: return '3';
    case 56: return '4';
    case 57: return '5';
    case 58: return '6';
    case 59: return '7';
    case 60: return '8';
    case 61: return '9';
    case 62: return '+';
    case 63: return '/';
    default: return '?';
  }
}

} // namespace

} // namespace marslander::base64
