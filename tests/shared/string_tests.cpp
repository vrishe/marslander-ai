#include "shared.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "gtest/gtest.h"
namespace {

using namespace marslander;
using namespace std;

TEST(SharedTests, string_split_simple) {

  auto s = split("Hello, world!"s, " "s);
  auto expected = vector<string> { "Hello,", "world!" };

  ASSERT_EQ(expected.size(), s.size());
  for (size_t i = 0; i < expected.size(); ++i)
    ASSERT_EQ(expected[i], s[i]);

}

TEST(SharedTests, string_split_multi_delim) {

  auto s = split("Hello, world! Bye-Bye, world!"s, " ,!-"s, true);
  auto expected = vector<string> { "Hello", "world", "Bye", "Bye", "world" };

  ASSERT_EQ(expected.size(), s.size());
  for (size_t i = 0; i < expected.size(); ++i)
    ASSERT_EQ(expected[i], s[i]);

}

} // namespace
