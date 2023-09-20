#include "nn.h"

#include <cmath>

#include "gtest/gtest.h"
namespace {

using namespace marslander::nn;

struct TestDFF {

  layer<double, layer_meta<2, 4>> hidden0;
  layer<double, layer_meta<4, 2>> hidden1;
  layer<double, layer_meta<2, 1>> output;

  typedef decltype(TestDFF::hidden0)::in_type in_type;

  auto operator()(const in_type& input) const {
    return apply(tanh, output(
      apply(tanh, hidden1(
        apply(tanh, hidden0(input))
      ))
    ));
  }

};

TestDFF build_nn() {
  return TestDFF {
    {
      { -.11, -.18, .041, -.26 },
      {
        1.2,     .048,
        -.052, -1.6,
         .096,   .98,
        1.5,     .05,
      },
    },
    {
      { 1.5, 1.4 },
      {
         .89, 1.3,  -.86,  1.3,
        -.9, -1.3,   .6,  -1.1,
      },
    },
    {
      { 0 },
      {
        2.5, 2.5,
      },
    }
  };
}

TEST(NNTests, Exhausting) {
  auto net = build_nn();

  typedef matrix<double, 2, 1> vec;
  auto result = net(vec { 3, 3 });
  EXPECT_GT(result.value_at(0) * 2 - 1, 0);

  result = net(vec { 3, -2 });
  EXPECT_LT(result.value_at(0) * 2 - 1, 0);

  result = net(vec { -4, -5 });
  EXPECT_GT(result.value_at(0) * 2 - 1, 0);

  result = net(vec { -3, 4 });
  EXPECT_LT(result.value_at(0) * 2 - 1, 0);
}

}
