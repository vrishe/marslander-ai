#include "linalg.h"

#include "gtest/gtest.h"
namespace {

using namespace marslander::linalg;

template<class M>
bool are_equal(const M& a, const M& b) {
  for (auto r = 0; r < a.rows(); ++r) {
    for (auto c = 0; c < b.cols(); ++c) {
      if (a.value_at(r,c) != b.value_at(r,c))
        return false;
    }
  }

  return true;
}

TEST(LinalgTests, ColRow) {
  matrix<int, 3, 4> m {
    1,  2,  3,  4,
    5,  6,  7,  8,
    9, 10, 11, 12,
  };

  EXPECT_TRUE(are_equal(m.col(2),
    matrix<int, 3, 1> {
       3,
       7,
      11,
    }));

  EXPECT_TRUE(are_equal(m.row(1),
    matrix<int, 1, 4> {
       5,  6,  7,  8,
    }));
}

int negate(int v) noexcept { return -v; }

TEST(LinalgTests, Apply) {
  const matrix<int, 2, 3> a {
    1,  3,  5,
    7,  9, 11,
  };

  EXPECT_TRUE(are_equal(apply(negate, a),
    matrix<int, 2, 3> {
      -1,  -3,  -5,
      -7,  -9, -11,
    }));

  matrix<int, 3, 2> b {
     2,  4,
     6,  8,
    10, 12,
  };

  EXPECT_TRUE(are_equal(apply(negate, b),
    matrix<int, 3, 2> {
       -2,  -4,
       -6,  -8,
      -10, -12,
    }));

  EXPECT_TRUE(are_equal(apply(negate, std::move(b)),
    matrix<int, 3, 2> {
        2,   4,
        6,   8,
       10,  12,
    }));
}

TEST(LinalgTests, Mul_Non_Square) {
  matrix<int, 2, 3> a {
    1, 0, 0,
    0, 0, 1,
  };

  matrix<int, 3, 4> b {
    1,  2,  3,  4,
    5,  6,  7,  8,
    9, 10, 11, 12,
  };

  EXPECT_TRUE(are_equal(mul(a, b),
    matrix<int, 2, 4> {
      1,  2,  3,  4,
      9, 10, 11, 12,
    }));
}

TEST(LinalgTests, Mul_Square) {
  matrix<int, 3, 3> a {
    2, 0, 0,
    0, 4, 0,
    0, 0, 8,
  };

  matrix<int, 3, 3> b {
    1, 1, 1,
    3, 3, 3,
    5, 5, 5,
  };

  EXPECT_TRUE(are_equal(mul(a, b),
    matrix<int, 3, 3> {
       2,  2,  2,
      12, 12, 12,
      40, 40, 40,
    }));
}

TEST(LinalgTests, Normalized) {
  matrix<float, 3, 3> a {
    2, 0, 0,
    0, 4, 0,
    0, 0, 8,
  };

  // Blant FP equality check is questionable
  EXPECT_TRUE(are_equal(normalized(a),
    matrix<float, 3, 3> {
      1, 0, 0,
      0, 1, 0,
      0, 0, 1,
    }));

  matrix<float, 3, 3> b {
    1, 3, 5,
    1, 3, 5,
    1, 3, 5,
  };

  auto c0 = 1 / std::sqrt( 3.f);
  auto c1 = 3 / std::sqrt(27.f);
  auto c2 = 5 / std::sqrt(75.f);

  EXPECT_TRUE(are_equal(normalized(b),
    matrix<float, 3, 3> {
      c0, c1, c2,
      c0, c1, c2,
      c0, c1, c2,
    }));
}

TEST(LinalgTests, op_Add) {
  matrix<int, 2, 3> a {
    1,  3,  5,
    7,  9, 11,
  };

  matrix<int, 2, 3> b {
    2,  4,  6,
    8, 10, 12,
  };

  EXPECT_TRUE(are_equal(a + b,
    matrix<int, 2, 3> {
       3,  7, 11,
      15, 19, 23,
    }));
}

TEST(LinalgTests, op_Mul) {
  matrix<int, 2, 3> a {
    1,  3,  5,
    7,  9, 11,
  };

  matrix<int, 2, 3> b {
    2,  4,  6,
    8, 10, 12,
  };

  EXPECT_TRUE(are_equal(a * b,
    matrix<int, 2, 3> {
       2,  12,  30,
      56,  90, 132,
    }));
}

}
