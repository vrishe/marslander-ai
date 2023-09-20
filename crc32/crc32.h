#pragma once

#ifndef CRC32_H_
#define CRC32_H_

#include <cstddef>
#include <cstdint>

namespace marslander::checksum {

typedef uint32_t crc32_t;

class crc32 final {

  crc32_t _c;

public:

  crc32() : _c{} {};

  crc32(crc32_t c) : _c{c} {}

  crc32(const crc32&) = default;

  crc32(crc32&&) = default;

  crc32_t update(const void* buf, size_t size);

  crc32_t checksum() const noexcept { return _c; }

  operator crc32_t() const noexcept { return _c; }

  void reset(crc32_t c = crc32_t{}) noexcept { _c = c; }

};

} // namespace marslander

#endif // CRC32_H_
