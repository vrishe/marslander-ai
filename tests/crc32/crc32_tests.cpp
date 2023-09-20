#include "crc32.h"

#include <iomanip>
#include <iterator>

#include "gtest/gtest.h"
namespace marslander::checksum {

// See: https://crccalc.com/

namespace {

uint32_t crc32_ref(const void* M, uint32_t bytes) {
  // This is CRC-32C poly
  // which x86 hardware instructions hardcode internally.
  static constexpr uint32_t P = 0x82f63b78U;

  const uint8_t* M8 = reinterpret_cast<const uint8_t*>(M);

  uint32_t R = ~uint32_t{};
  for (uint32_t i = 0; i < bytes; ++i) {
    R ^= M8[i];
    for (uint32_t j = 0; j < 8; ++j)
      R = R & 1 ? (R >> 1) ^ P : R >> 1;
  }

  return ~R;
}

static constexpr char const buf_large_4k_[] =
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum ac ex ac"
" felis maximus rutrum. Duis fermentum leo id fringilla sagittis. Integer"
" lobortis nibh mauris, sed aliquam leo vulputate eget. Quisque ut ultrices"
" augue, in vehicula nisl. Proin sit amet mauris orci. Donec eu faucibus leo."
" Pellentesque eget ipsum et justo hendrerit pellentesque. Vivamus ullamcorper"
" tincidunt est. Maecenas quam quam, luctus eu velit sed, mollis ultricies"
" odio. Donec erat mauris, vestibulum a elit quis, pulvinar viverra lorem."
" Etiam molestie arcu ac arcu egestas, sed laoreet diam lacinia. Proin et mi"
" in ligula tincidunt congue."

"Etiam vitae nibh nisl. Nullam quis mi rutrum, suscipit neque in, efficitur"
" quam. In porta condimentum orci id congue. Nulla at dui odio. Proin ac"
" urna eu massa tempor scelerisque. Donec non molestie quam, nec bibendum"
" massa. Vivamus imperdiet ac orci sed placerat. Nam a tincidunt arcu,"
" molestie ultricies lacus. Nulla dapibus nisi in libero sollicitudin"
" venenatis. Sed urna risus, fringilla eu nunc ac, porttitor sodales diam."
" Aliquam eros urna, sodales a finibus et, laoreet et neque."

"Aenean nec pellentesque erat. Nam eget dignissim nisl. Vivamus gravida tempor"
" rutrum. Nam tincidunt suscipit sodales. Etiam quis enim hendrerit, elementum"
" nisi ut, viverra eros. Praesent rhoncus consectetur erat nec rutrum. Ut"
" pretium enim ac ante tristique efficitur. Nullam scelerisque erat ante, at"
" malesuada sem feugiat eget. Suspendisse accumsan mollis ullamcorper. Ut"
" lacinia erat eget mi fermentum laoreet. Curabitur sagittis malesuada ante,"
" sit amet fermentum turpis eleifend in. Praesent nec velit placerat, pulvinar"
" mi in, suscipit nisl. Duis vitae odio sollicitudin, venenatis sapien sed,"
" ultricies justo."

"Phasellus id urna elit. Aenean euismod massa lectus, in faucibus neque"
" aliquam ut. In sed tempor metus, eget hendrerit massa. Sed leo nunc,"
" pellentesque vel gravida sed, commodo a elit. Integer a enim nec dui"
" vehicula tincidunt. Nulla mattis feugiat tincidunt. Vivamus consequat tortor"
" odio, a convallis nisl rhoncus in. Maecenas mattis feugiat malesuada."

"Maecenas laoreet massa suscipit, faucibus nisl ut, hendrerit erat. Aenean"
" orci magna, scelerisque in ultricies id, vestibulum ac dolor. Quisque"
" aliquet placerat purus, vel sodales massa bibendum at. Nullam et nulla in"
" purus dignissim facilisis. Quisque sed eros efficitur, congue est sed,"
" eleifend massa. Nullam aliquam ac dui ac varius. Lorem ipsum dolor sit amet,"
" consectetur adipiscing elit. Nulla facilisi. Vestibulum venenatis ex erat,"
" vitae gravida nisl finibus non. Morbi fringilla ligula in turpis convallis,"
" eget feugiat ante convallis."

"Cras pharetra lacus sed augue euismod posuere eu nec lacus. Phasellus ut"
" euismod sapien. Sed ultricies, tellus non auctor euismod, odio magna"
" scelerisque purus, vitae eleifend magna odio ac eros. Vestibulum ante ipsum"
" primis in faucibus orci luctus et ultrices posuere cubilia curae; Sed"
" placerat venenatis augue. Donec vel lobortis augue. Class aptent taciti"
" sociosqu ad litora torquent per conubia nostra, per inceptos himenaeos."
" Mauris lobortis molestie fringilla. In eu luctus metus. Nam pharetra tellus"
" vel urna suscipit dictum. Proin porttitor dolor eu consectetur dapibus."
" Quisque in risus vitae nunc elementum ornare at at nibh. Nunc ut mauris"
" massa. Aenean ac tellus commodo felis dapibus aliquet semper id nulla."
" Quisque lacus velit, venenatis a dui quis, accumsan pharetra purus. Nunc"
" aliquet, ex ut egestas hendrerit, lectus lacus mattis nibh, ac iaculis"
" tellus ante eu tortor."

"Quisque efficitur dictum congue. Proin vestibulum, nunc id pellentesque"
" mattis, mi neque auctor nisi, semper pulvinar eros lectus at lectus."
" Nulla turpis tortor, vehicula quis ullamcorper sed, molestie sed turpis."
" Integer molestie ullamcorper venenatis. Mauris augue velit, consequat eget"
" tellus sed, tempor laoreet erat. Sed sodales turpis vitae gravida laoreet."
" Aliquam erat volutpat. Curabitur eget sem at erat rhoncus blandit. Maecenas"
" tincidunt ex metus, et egestas nisi finibus vitae. Suspendisse feugiat, dui"
" ut scelerisque dignissim, justo enim ornare purus, vel porttitor.";

} // namespace

TEST(CRC32Tests, ChecksumCorrectness) {
  crc32 checksum;

  constexpr auto data = "123456789";
  auto result = checksum.update(data, 9);
  ASSERT_EQ(0xE3069283, result);
}

TEST(CRC32Tests, MultipleUpdates) {
  crc32 checksum;

  constexpr auto data = "123456789";
  checksum.update(data, 3);
  checksum.update(&data[3], 3);
  auto result = checksum.update(&data[6], 3);
  ASSERT_EQ(0xE3069283, result);
}

TEST(CRC32Tests, LargeBuffer) {
  crc32 checksum;

  auto result = checksum.update(buf_large_4k_, sizeof(buf_large_4k_));
  const auto expected = crc32_ref(buf_large_4k_, sizeof(buf_large_4k_));
  ASSERT_EQ(expected, result);
}

TEST(CRC32Tests, LargeBufferMultipleUpdates) {
  crc32 checksum;

  auto buf = buf_large_4k_;
  size_t upd_sz = 1,
         buf_sz = sizeof(buf_large_4k_);
  do {
    checksum.update(buf, upd_sz);
    buf += upd_sz; buf_sz -= upd_sz;
    upd_sz <<= 1;
  }
  while (buf_sz && buf_sz > upd_sz);

  auto result = checksum.update(buf, buf_sz);
  const auto expected = crc32_ref(buf_large_4k_, sizeof(buf_large_4k_));
  ASSERT_EQ(expected, result);
}

} // namespace marslander::checksum
