#include "marslander/state.h"

#include "gtest/gtest.h"
namespace marslander {

TEST(SharedTests, state_to_base64_roundtrip) {
  marslander::state src {
    {
      { {0, 0}, {1, 3}, {2, 3} },
      { 1, 2 },
    },
    999, 1, -15,
    { 22, 33 },
    { 55, -13 },
    { 1, 2 },
    3,
    {}
  };

  auto data = src.to_base64();

  marslander::state src_copy;
  src_copy.from_base64(data);

  auto data_copy = src_copy.to_base64();

  ASSERT_TRUE(data == data_copy);
}

} // namespace marslander
