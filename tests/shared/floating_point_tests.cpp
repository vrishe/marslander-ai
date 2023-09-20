// #define LONG_RUNNING
#include "shared.h"

#include "floating_point/ExactSum.h"

#include <random>
#include <sstream>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

namespace marslander::fp {

extern double well_conditioned_sum;
extern double random_sum;
extern double andersons_sum;
extern double sum_zero_sum;

extern std::vector<double> well_conditioned;
extern std::vector<double> random;
extern std::vector<double> andersons;
extern std::vector<double> sum_zero;

} // namespace marslander::fp

#define TABLE_TEST_(f, t) \
TEST(SharedTests, fp_ ## f ## _ ## t) {\
  using namespace marslander;\
  ASSERT_DOUBLE_EQ(fp::t ## _sum, fp::f(fp::t.begin(), fp::t.end()));\
}

#define TABLE_TEST_ACCUMULATOR_(t) \
TEST(SharedTests, fp_accumulator_ ## t) {\
  using namespace marslander;\
  fp::accumulator acc;\
  size_t n, i{}, table_sz = fp::t.size();\
  while (table_sz & 7) {table_sz--; acc.add(fp::t[i++]);}\
  n = table_sz >> 3; while (n--) acc.add(fp::t[i++]);\
  n = table_sz >> 3; acc.add(&fp::t[i], &fp::t[i+n]); i+=n;\
  n = table_sz >> 2; acc.add(&fp::t[i], &fp::t[i+n]); i+=n;\
  n = table_sz >> 1; acc.add(&fp::t[i], &fp::t[i+n]);\
  ASSERT_DOUBLE_EQ(fp::t ## _sum, acc.result());\
}

namespace {

TEST(SharedTests, fp_accumulator_Inf) {
  using namespace marslander;

  fp::accumulator acc;
  acc.add( std::numeric_limits<double>::infinity());
  acc.add(-std::numeric_limits<double>::infinity());
  ASSERT_TRUE(std::isnan(acc.result()));
}

TEST(SharedTests, fp_accumulator_NaN) {
  using namespace marslander;

  std::vector<double> values = {
    4.444,
    -2.33e-10,
    5.6223,
    std::numeric_limits<double>::quiet_NaN(),
    10,
    -1.1353423e12
  };

  fp::accumulator acc;
  acc.add(values.begin(), values.end());
  auto result = acc.result();
  ASSERT_TRUE(std::isnan(result));
}

TABLE_TEST_ACCUMULATOR_(well_conditioned)
TABLE_TEST_ACCUMULATOR_(random)
TABLE_TEST_ACCUMULATOR_(andersons)
TABLE_TEST_ACCUMULATOR_(sum_zero)


TEST(SharedTests, fp_fast_sum_Inf) {
  using namespace marslander;

  double v[] = { 
    -std::numeric_limits<double>::infinity(),
     std::numeric_limits<double>::infinity()
  };

  auto actual = fp::fast_sum(v, v + 2);
  ASSERT_TRUE(std::isnan(actual));
}

TEST(SharedTests, fp_fast_sum_NaN) {
  using namespace marslander;

  std::vector<double> values = {
    4.444,
    -2.33e-10,
    std::numeric_limits<double>::quiet_NaN(),
    5.6223,
    10,
    -1.1353423e12
  };

  auto result = fp::fast_sum(values.begin(), values.end());
  ASSERT_TRUE(std::isnan(result));
}

TABLE_TEST_(fast_sum, well_conditioned)
TABLE_TEST_(fast_sum, random)
TABLE_TEST_(fast_sum, andersons)
TABLE_TEST_(fast_sum, sum_zero)


static constexpr uint64_t bitsize_(uint64_t v) {
  // expand lower bits
  v |= (v >> 1);
  v |= (v >> 2);
  v |= (v >> 4);
  v |= (v >> 8);
  v |= (v >> 16);
  v |= (v >> 32);
  // calculate set bits count
  v -= (v >> 1) & 0x5555555555555555;
  v = (v & 0x3333333333333333) + ((v >> 2) & 0x3333333333333333);
  v = (v & 0x0F0F0F0F0F0F0F0F) + ((v >> 4) & 0x0F0F0F0F0F0F0F0F);
  return (((v + (v >> 8)) & 0x00FF00FF00FF00FF) * 0x0001000100010001) >> 48;
}

template<class RNG>
double next_double(RNG&& rng, unsigned exponent = 0) {
  uint64_t value{};
  {
    using re = std::decay_t<RNG>;
    constexpr uint64_t one{1};
    constexpr auto ofs = bitsize_(re::max() - re::min());
    constexpr uint64_t mask{(one << ofs) - one};
    for (size_t i = 0; i < 64; i+=ofs)
      value = (value << ofs) | (rng() & mask);
  }

  value &= 0x800FFFFFFFFFFFFF;
  if (exponent) {
    std::uniform_int_distribution<unsigned> d{exponent};
    value |= (uint64_t{0x3FF} + d(rng) % exponent - (exponent >> 1)) << 52;
  }
  else
    value |= 0x3FF0000000000000;

  return *reinterpret_cast<double*>(&value);
}

template<class RNG, class OutIt>
void generate_doubles(RNG&& rng, size_t n, unsigned exponent,
    bool zero_sum, OutIt dst) {

  if (zero_sum) {
    for (auto new_n = n & ~size_t{1}; n > new_n; --n)
      *dst++ = .0;
  }

  bool negate{};
  double last_val;
  while (n--) {
    negate &= zero_sum;
    *dst++ = (negate = !negate) 
      ? (last_val = next_double(rng, exponent))
      : -last_val;
  }
}

enum class double_rnd_mode {
  well_conditioned,
  random,
  andersons,
  sum_zero
};

template<class RNG>
void generate_doubles_vector(RNG&& rng, size_t n, unsigned exponent,
    double_rnd_mode mode, std::vector<double>& out_doubles) {

  out_doubles.clear();
  out_doubles.reserve(n);
  generate_doubles(rng, n,
    exponent, mode == double_rnd_mode::sum_zero,
    std::back_inserter(out_doubles));

  switch (mode) {
    case double_rnd_mode::andersons: {
      auto avg = accumulate(out_doubles.begin(), out_doubles.end(), .0) 
        / out_doubles.size();
      for (auto& v : out_doubles)
        v -= avg;
      break;
    }
    case double_rnd_mode::sum_zero: {
      auto out_doubles_it = out_doubles.begin();
      if (n & 1) out_doubles_it++;
      std::shuffle(out_doubles_it, out_doubles.end(), rng);
      break;
    }
  }
}

TEST(SharedTests, fp_fast_sum_referential) {
  using namespace marslander;

  double_rnd_mode modes[] = {
    double_rnd_mode::well_conditioned,
    double_rnd_mode::random,
    double_rnd_mode::andersons,
    double_rnd_mode::sum_zero
  };

  unsigned deltas[] = {
    10, 30, 50, 1000
  };

  std::mt19937 rng(std::random_device{}());

  constexpr size_t values_sz = 1000;
  std::vector<double> values;
  for (size_t n = 160; n > 0; --n) {
    for (auto mode : modes) {
      for (auto delta : deltas) {
        std::stringstream rng_state;
        rng_state << rng;

        generate_doubles_vector(rng, values_sz + 1, delta, mode, values);

        ExactSum es;
        auto copy{values};
        ASSERT_DOUBLE_EQ(es.iFastSum(&copy[0], values_sz),
            fp::fast_sum(std::next(values.begin()), values.end()))
          << "m: " << static_cast<int>(mode) << " exp: " << delta
          << "\nrng: " << rng_state.str();
      }
    }
  }
}

#ifdef LONG_RUNNING

TEST(SharedTests, fp_accumulator_sustained) {
  using namespace marslander;

  std::mt19937 rng(std::random_device{}());

  ExactSum es;
  fp::accumulator acc;

  constexpr size_t values_sz = 32768;
  std::vector<double> values;
  for (size_t N = 3072; N > 0; --N) {
    generate_doubles_vector(rng, values_sz + 1, 5,
      double_rnd_mode::andersons, values);

    es.AddArray(&values[0], values_sz - 2);
    es.AddNumber(values[values_sz - 1]);
    es.AddNumber(values[values_sz]);

    auto it = std::next(values.begin());
    acc.add(*it++);
    acc.add(*it++);
    acc.add(it, values.end());
  }

  auto expected = es.GetSum();
  auto actual = acc.result();

  ASSERT_FALSE(std::isnan(expected));
  ASSERT_FALSE(std::isnan(actual));
  ASSERT_DOUBLE_EQ(expected, actual);
}

#endif // LONG_RUNNING

} // namespace
