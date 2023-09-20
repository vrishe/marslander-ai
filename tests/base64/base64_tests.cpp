#include "base64.h"

#include <cstring>

#include "gtest/gtest.h"
namespace marslander::base64 {

namespace {

const char plain_text_short_[]   = "Hello, world!";
const char encoded_text_short_[] = "SGVsbG8sIHdvcmxkIQ==";

const char plain_text_[] =
"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Aenean lacinia "
"accumsan pellentesque. Phasellus consequat lorem nec odio ultricies "
"rutrum. Pellentesque non elit eget turpis imperdiet mollis. Suspendisse "
"in risus quis tortor ornare varius. Nullam vitae eleifend elit, quis "
"iaculis eros. Fusce scelerisque rhoncus tristique. Nullam sollicitudin "
"a mi at luctus. Ut leo velit, cursus quis posuere eget, consequat id "
"enim. Mauris diam tortor, ornare quis orci et, pharetra feugiat ante. "
"Nunc condimentum, magna nec viverra scelerisque, erat nunc ullamcorper "
"orci, nec auctor elit mauris nec lacus. Sed imperdiet felis a lacinia "
"dignissim. Ut tincidunt sollicitudin orci sit amet gravida. Nunc nec "
"nunc a velit porta molestie vel sit amet nunc.";

} // namespace

#define BAD_PTR(T) reinterpret_cast<T*>(0x0BAD0BAD)
#define RCAST(T, v) reinterpret_cast<T*>(v)

TEST(BASE64Tests, decode_prereq) {
  ASSERT_EQ(13, base64::decode(nullptr, 13, BAD_PTR(unsigned char), 20));
  ASSERT_EQ( 0, base64::decode(nullptr,  0, BAD_PTR(unsigned char), 20));
}

TEST(BASE64Tests, decode_length_estimate) {
  ASSERT_EQ(strlen(plain_text_short_),
    base64::decode(encoded_text_short_, strlen(encoded_text_short_),
      nullptr, 0));
}

TEST(BASE64Tests, decode_short_text) {
  char dst[strlen(plain_text_short_) + 1];
  ASSERT_EQ(0, base64::decode(encoded_text_short_, strlen(encoded_text_short_),
    RCAST(unsigned char, dst), strlen(plain_text_short_)));
  ASSERT_EQ(0, strncmp(dst, plain_text_short_, strlen(plain_text_short_)));
}

TEST(BASE64Tests, encode_prereq) {
  ASSERT_EQ(13, base64::encode(nullptr, 13, BAD_PTR(char), 20));
  ASSERT_EQ( 0, base64::encode(nullptr,  0, BAD_PTR(char), 20));
}

TEST(BASE64Tests, encode_length_estimate) {
  ASSERT_EQ(strlen(encoded_text_short_), base64::encode(
    RCAST(const unsigned char, plain_text_short_),
      strlen(plain_text_short_), nullptr, 0));
}

TEST(BASE64Tests, encode_short_text) {
  char dst[strlen(encoded_text_short_) + 1];
  ASSERT_EQ(0, base64::encode(
    RCAST(const unsigned char, plain_text_short_),
      strlen(plain_text_short_), dst, strlen(encoded_text_short_)));
  ASSERT_EQ(0, strncmp(dst, encoded_text_short_, strlen(encoded_text_short_)));
}

TEST(BASE64Tests, encode_decode_round_trip) {
  size_t plain_text_len = strlen(plain_text_);
  size_t encoded_text_len = base64::encode(
    RCAST(const unsigned char, plain_text_), plain_text_len, nullptr, 0);

  std::string dst(plain_text_len + encoded_text_len, '#');
  base64::encode(RCAST(const unsigned char, plain_text_),
    plain_text_len, dst.data() + plain_text_len, encoded_text_len);
  base64::decode(dst.data() + plain_text_len, encoded_text_len,
    RCAST(unsigned char, dst.data()), plain_text_len);

  ASSERT_EQ(0, strncmp(plain_text_, dst.data(), plain_text_len));
}

} // namespace marslander::base64
