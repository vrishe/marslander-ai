#pragma once

#ifndef BASE64_H_
#define BASE64_H_

#include <cstddef>
#include <cstdint>

namespace marslander::base64 {

size_t decode(const char*, size_t, unsigned char*, size_t);
size_t encode(const unsigned char*, size_t, char*, size_t);

} // namespace marslander::base64

#endif // BASE64_H_
