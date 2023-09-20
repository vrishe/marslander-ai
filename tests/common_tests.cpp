#include "common.h"

#include <sstream>

namespace marslander {

bool operator==(const ipoint& a, const ipoint& b) {
  return a.x == b.x && a.y == b.y;
}

}

#include "gtest/gtest.h"
namespace {

using namespace marslander;
using namespace std;

TEST(CommonTests, op_Extract_Point) {
  stringstream ss("   15  39    ");

  ipoint p{};
  ss >> p;

  EXPECT_EQ((ipoint { 15, 39 }), p);
}

TEST(CommonTests, op_Insert_Point) {
  stringstream ss{};

  ipoint p{15, 48};
  ss << p;

  EXPECT_EQ("{ 15, 48 }", ss.str());
}

TEST(CommonTests, op_Extract_Vector) {
  stringstream ss("  3  15 53    6 4   20 23    ");

  ipoints p{};
  ss >> p;

  EXPECT_EQ(3, p.size());
  EXPECT_EQ((ipoint { 15, 53 }), p[0]);
  EXPECT_EQ((ipoint {  6,  4 }), p[1]);
  EXPECT_EQ((ipoint { 20, 23 }), p[2]);
}

TEST(CommonTests, op_Insert_Vector) {
  stringstream ss{};

  ipoints p { { 16, 10 }, { 6, 4 }, { 20, 23 } };
  ss << p;

  EXPECT_EQ("{ 16, 10 }, { 6, 4 }, { 20, 23 }", ss.str());
}

}
