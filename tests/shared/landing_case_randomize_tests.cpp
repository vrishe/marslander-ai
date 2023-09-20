#include "shared.h"
#include "common.h"

#include <random>

namespace marslander::pb {

using namespace std;

ostream& operator<<(ostream& dst, const landing_case& tc) {
  for (auto& p : tc.surface())
    dst << "(" << p.x() << ", " << p.y() << "), ";
  dst << endl;
  dst << tc.safe_area().start() << ", " << tc.safe_area().end() << endl;
  return dst;
}

} // namespace marslander::pb

#include "gtest/gtest.h"
namespace {

::testing::AssertionResult is_within_incl(int val, int a, int b)
{
  if((a <= val) && (val <= b))
    return ::testing::AssertionSuccess();
  else
    return ::testing::AssertionFailure()
      << val << " is outside the range " << a << " to " << b;
}

template<typename Vec2d>
auto sqr_dist(const Vec2d& v) {
  return v.x()*v.x() + v.y()*v.y();
}

using namespace marslander;
using namespace std;

class SharedTests_landing_case_randomize : public ::testing::Test {

  random_device _rd;
  mt19937 _rng;

protected:
  void SetUp() override {
    _rng = mt19937(_rd());
  }

  void test_randomize_once() {
    pb::landing_case tc;
    landing_case::randomize(_rng, tc);

    ASSERT_GT(tc.fuel(), 0);
    ASSERT_LE(tc.fuel(), constants::fuel_amount_max);

    ASSERT_EQ(tc.thrust(), 0);

    auto pos = tc.position();
    ASSERT_GE(pos.x(), landing_case::start_position_horz_padding);
    ASSERT_GE(constants::zone_x_max - pos.x(),
       landing_case::start_position_horz_padding);

    ASSERT_LE(pos.y(), landing_case::start_position_altitude_max);
    ASSERT_GE(pos.y(), landing_case::start_position_altitude_min);

    ASSERT_LE(tc.tilt(), constants::tilt_angle_max);
    ASSERT_GE(tc.tilt(), constants::tilt_angle_min);

    auto vel_sqr = sqr_dist(tc.velocity());
    if (vel_sqr > 0) {
      ASSERT_DOUBLE_EQ(tc.velocity().y(), 0);

      auto speed = sqrt(vel_sqr);
      ASSERT_LE(speed, landing_case::initial_speed_max);
      ASSERT_GE(speed, landing_case::initial_speed_min);
    }

    auto safe_area = tc.safe_area();
    ASSERT_EQ(safe_area.end() - safe_area.start(), 1);
    ASSERT_GE(tc.surface(safe_area.start()).x(), 0);
    ASSERT_LE(tc.surface(safe_area.end()).x(), constants::zone_x_max);
    ASSERT_GE(tc.surface(safe_area.end()).x() - tc.surface(safe_area.start()).x(),
      constants::surface_flat_width_min);

    inum safe_area_alt_end = tc.surface(safe_area.end()).y(),
         safe_area_alt_start = tc.surface(safe_area.start()).y();

    ASSERT_EQ(safe_area_alt_end, safe_area_alt_start);
    ASSERT_LE(safe_area_alt_end, landing_case::surface_flat_elevation_max);
    ASSERT_GE(safe_area_alt_end, landing_case::surface_flat_elevation_min);

    auto surface_size = tc.surface_size();
    ASSERT_GE(surface_size, landing_case::surface_points_count_min);
    ASSERT_LE(surface_size, landing_case::surface_points_count_max);
    ASSERT_EQ(tc.surface(0).x(), 0);
    ASSERT_EQ(tc.surface(surface_size - 1).x(), constants::zone_x_max);

    auto x_prev = -1;
    for (auto& p : tc.surface()) {
      ASSERT_GT(p.x(), x_prev);
      ASSERT_LE(p.y(), landing_case::surface_elevation_max);
      x_prev = p.x();

      if (p.x() == safe_area.end()) safe_area_alt_end = p.y();
      if (p.x() == safe_area.start()) safe_area_alt_start = p.y();
    }
  }
};

TEST_F(SharedTests_landing_case_randomize, _4K_times) {
  for (auto i = 1; i <= 4096; ++i) {
    EXPECT_NO_FATAL_FAILURE(test_randomize_once()) << "iteration " << i;
  }
}

}
