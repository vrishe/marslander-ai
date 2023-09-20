#include "shared.h"
#include "common.h"

#include <iterator>
#include <tuple>
#include <random>
#include <vector>

#include "gtest/gtest.h"
namespace {

template<typename... T>
std::array<typename std::common_type<T...>::type, sizeof...(T)>
make_array(T &&...t) { return {std::forward<T>(t)...}; }

template<typename Iter>
auto stddev(Iter it, Iter end) {
  using value_type = typename std::iterator_traits<Iter>::value_type;
  const auto n = std::distance(it, end);
  auto avg = std::accumulate(it, end, value_type(0)) / n;
  value_type sigma_sqr_over_n(0);
  while (it != end) {
    auto d = *it++ - avg;
    sigma_sqr_over_n += d*d;
  }
  return sqrt(sigma_sqr_over_n / n);
}

template<typename Iter>
bool all_zeroes(Iter it, Iter end) {
  constexpr auto Z = typename std::iterator_traits<Iter>::value_type(0);
  while (it != end)
    if (*it++ != Z) return false;

  return true;
}

template<typename Iter, typename P>
bool all_pred(Iter it, Iter end, P pred) {
  while (it != end && pred(*it++)) {};
  return it == end;
}

using namespace marslander;
using namespace std;

#define LAYER_TUPLE(name) make_tuple(\
  nn::DFF_meta::name ## _meta::layer_weights_offset,\
  nn::DFF_meta::name ## _meta::layer_size,\
  nn::DFF_meta::name ## _meta::input_size,\
  nn::DFF_meta::name ## _meta::neurons_count,\
  #name)

TEST(SharedTests, inner_stddev) {
  auto ref = make_array(2., 4., 4., 4., 5., 5., 7., 9.);
  ASSERT_DOUBLE_EQ(stddev(begin(ref), end(ref)), 2.);
}

TEST(SharedTests, nn_randomize_for_ReLU) {
  random_device rd;
  mt19937 rng(rd());

  pb::genome g;
  nn::randomize<nn::activators::ReLU>(rng, g);

  auto v = g.genes();
  auto ts = make_array(
    LAYER_TUPLE(hidden0),
    LAYER_TUPLE(hidden1),
    LAYER_TUPLE(output )
  );

  auto v_it = v.begin();
  for (auto t : ts) {
    ASSERT_TRUE(all_zeroes(v_it, v_it + get<0>(t))) << get<4>(t);

    auto expected = sqrt(fnum(2)/get<2>(t));
    auto stddev_v = stddev(v_it, v_it + get<1>(t));
    EXPECT_NEAR(stddev_v, expected, expected) << get<4>(t);

    v_it += get<1>(t);
  }
}

TEST(SharedTests, nn_randomize_for_tanh) {
  random_device rd;
  mt19937 rng(rd());

  pb::genome g;
  nn::randomize<nn::activators::tanh>(rng, g);

  auto v = g.genes();
  auto ts = make_array(
    LAYER_TUPLE(hidden0),
    LAYER_TUPLE(hidden1),
    LAYER_TUPLE(output )
  );

  auto v_it = v.begin();
  for (auto t : ts) {
    ASSERT_TRUE(all_zeroes(v_it, v_it + get<0>(t))) << get<4>(t);

    auto l = sqrt(6)/sqrt(get<2>(t) + get<3>(t));
    auto all_within_range = all_pred(v_it, v_it + get<1>(t),
      [l](auto v) { return -l <= v && v <= l; });
    EXPECT_TRUE(all_within_range) << get<4>(t);

    v_it += get<1>(t);
  }
}

TEST(SharedTests, nn_randomize_naive) {
  random_device rd;
  mt19937 rng(rd());

  pb::genome g;
  nn::randomize_naive(rng, g);

  auto v = g.genes();
  ASSERT_TRUE(all_pred(v.begin(), v.end(),
    [](auto x) { return -1 <= x && x <= 1; }));
}

} // namespace
