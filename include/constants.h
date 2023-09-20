#pragma once

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include "common.h"

namespace marslander::constants {

  inline constexpr fnum mars_gravity_acc       = -3.711; // m/s^2

  inline constexpr inum fuel_amount_max        =  2000;  // L
  inline constexpr inum speed_limit_horz       =  20;    // m/s
  inline constexpr inum speed_limit_vert       =  40;    // m/s
  inline constexpr inum surface_flat_width_min =  1000;  // m
  inline constexpr inum thrust_delta_abs       =  1;     // m/s^3
  inline constexpr inum thrust_power_max       =  4;     // m/s^2
  inline constexpr inum thrust_power_min       =  0;     // m/s^2
  inline constexpr inum tilt_angle_max         =  90;    // deg
  inline constexpr inum tilt_angle_min         = -90;    // deg
  inline constexpr inum tilt_delta_abs         =  15;    // deg/s
  inline constexpr inum zone_height            =  3000;  // m
  inline constexpr inum zone_width             =  7000;  // m

  inline constexpr inum zone_x_max             = zone_width  - 1; // m
  inline constexpr inum zone_y_max             = zone_height - 1; // m

} // namespace marslander::constants

#endif // CONSTANTS_H_
