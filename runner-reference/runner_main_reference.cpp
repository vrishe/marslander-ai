#include "marslander/marslander.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <string>

using namespace std;

#define DEG2RAD (M_PI/180)
#define RAD2DEG (180/M_PI)

static bool solve_quad(float A, float B, float C, float &out_x1, float &out_x2) {
    float D = B*B - 4*A*C;
    if (D < 0)
        return false;
    
    out_x1 = (-B + sqrt(D)) / (2*A);
    out_x2 = (-B - sqrt(D)) / (2*A);

    return true;
}

template<class Func>
static float derivative(float a, float h, Func f) {
    return (f(a + h) - f(a)) / h;
}

// DOMAIN
#define MARS_GFORCE -3.711  // m/s^2
#define MAX_HSPEED   20     // m/s
#define MAX_VSPEED   40     // m/s
#define MAX_POWER    4      // m/s^2
#define MIN_POWER    1      // m/s^2
#define NO_POWER     0

namespace {

using namespace marslander;

static float surface_level(const state::surface_type& surface, float x) {
    if (surface.empty())
        return numeric_limits<float>::quiet_NaN();

    auto first = surface.begin(); if (x <= first->x) return first->y;
    auto last  = surface.end()-1; if (x >= last->x)  return last->y;

    using point_type = state::surface_type::value_type;
    auto hi = upper_bound(first, ++last, x,
        [](float value, const point_type& p) { 
            return p.x > value; 
        });
    auto lo = lower_bound(first, hi, x, 
        [](const point_type& p, float value) { 
            return p.x < value; 
        })-1;

    return lo->y + (x - lo->x) * (hi->y - lo->y) / float(hi->x - lo->x);
}

static float time_to_land(float a, float v0, float h) {
    float t1, t2;
    if (!solve_quad(.5f * a, v0, h, t1, t2)
            || t1 < 0 && t2 < 0)
        return numeric_limits<float>::quiet_NaN();

    if (t1 < 0) return t2;
    if (t2 < 0) return t1;
    
    return min(t1, t2);
}

void read_out(const state& s, int steps, int& X, int& Y,
      int& hSpeed, int& vSpeed, int& fuel, int& rotate, int& power) {
  X = static_cast<int>(s.position.x);
  Y = static_cast<int>(s.position.y);
  hSpeed = static_cast<int>(round(s.velocity.x));
  vSpeed = static_cast<int>(round(s.velocity.y));
  fuel = static_cast<int>(s.fuel);
  rotate = static_cast<int>(s.tilt);
  power = static_cast<int>(s.thrust);

  cout << "#" << steps << "|(" << X << ", " << Y << "),"
    << "|hspeed: " << hSpeed << "|vspeed: " << vSpeed
    << "|fuel: " << fuel << "|tilt: " << rotate << "|thrust: " << power << endl;
}

inline static state easy_on_the_right() {
  /*
   *  # Easy on the right
   *  landscape: [{0; 100}, {1000; 500}, {1500; 1500}, {3000; 1000}, {4000; 150}, {5500; 150}, {6999; 800}]
   *  position: 2500; 2700 hspeed: 0 vspeed: 0 fuel: 550 tile: 0 thrust: 0
   *  flat surface: { {4000; 150}; {5500; 150} }
   */
  return {
    // game_init_input
    {
      {
        {   0, 100},
        {1000, 500},
        {1500, 1500},
        {3000, 1000},
        {4000, 150},
        {5500, 150},
        {6999, 800}
      },
      { 4, 5 }
    }, 
    // game_turn_input
    {
      550, 0, 0,
      { 2500, 2700 },
      { 0, 0 }
    },
    // state
    { 4000, 5500 }, // safe area X
    {}
  };
}

inline static state initial_speed_wrong_side() {
  /*
   *  # Initial speed, wrong side
   *  landscape: [{0; 100}, {1000; 500}, {1500; 1500}, {3000; 1000}, {4000; 150}, {5500; 150}, {6999; 800}]
   *  position: 6500; 2800 hspeed: -90 vspeed: 0 fuel: 750 tile: 90 thrust: 0
   *  flat surface: { {4000; 150}; {5500; 150} }
   */
  return {
    // game_init_input
    {
      {
        {   0, 100},
        {1000, 500},
        {1500, 1500},
        {3000, 1000},
        {4000, 150},
        {5500, 150},
        {6999, 800}
      },
      { 4, 5 }
    },
    // game_turn_input
    {
      750, 0, 90,
      { 6500, 2800 },
      { -90, 0 }
    },
    // state
    { 4000, 5500 },
    {}
  };
}

void simulation(state& s, size_t steps_limit = 128)
{
  int X, Y;
  int hSpeed, vSpeed;
  int fuel;
  int rotate;
  int power;

  size_t steps = 0;
  for (; steps < steps_limit; ++steps) {
    read_out(s, steps, X, Y, hSpeed, vSpeed, fuel, rotate, power);

    auto h0 = surface_level(s.surface, X);
    auto k = 2*derivative(X - 500, 1000, [&s](float x) { 
        return surface_level(s.surface, x); 
    });
    auto power_ofs = abs(k) * MAX_POWER;
    s.out.tilt = atan(k) * RAD2DEG;

    // Approach #2
    auto a = s.thrust + MARS_GFORCE;
    auto v = vSpeed + a;
    auto h = Y + v - h0;

    auto tl = time_to_land(a, v, h);
    auto tb = (1-MAX_VSPEED - v) / (MAX_POWER + MARS_GFORCE);
    if (tb >= tl - 4) { // 4s is needed for thrusters to reach max power
        s.out.thrust = MAX_POWER;
    }
    else {
        s.out.thrust = NO_POWER;
    }
    s.out.thrust += power_ofs;
    s.out.thrust = clamp(s.out.thrust, NO_POWER, MAX_POWER);

    auto o = simulate(s);
    if (o != outcome::Aerial) {
      read_out(s, steps+1, X, Y, hSpeed, vSpeed, fuel, rotate, power);
      cout << (o == outcome::Crashed ? "Crash" : "Landing") << "!" << endl;
      return;
    }
  }

  cout << "STEPS LIMIT" << endl;
}

} // namespace

#define STATE_FACTORY(name) {#name,name}

int main(int argc, char* argv[])
{
  typedef state state_factory();
  using modes_map = map<string, state_factory*>;
  auto r = [](modes_map::iterator mode_it) {
    cout << "Running '" << mode_it->first << "'…" << endl;
    auto s = mode_it->second();
    simulation(s);
    cout << endl;
  };

  auto mode = modes_map {
    STATE_FACTORY(easy_on_the_right),
    STATE_FACTORY(initial_speed_wrong_side),
  };

  if (argc > 1) {
    auto i = 1;
    do {
      auto mode_it = mode.find(argv[i]);
      if (mode_it == mode.end()) {
        cerr << "Missing '" << argv[i] << "' mode" << endl;
        continue;
      }

      r(mode_it);
    } while (++i < argc);
  } else
    r(mode.begin());

  return 0;
}
