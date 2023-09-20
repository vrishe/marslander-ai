#pragma once

#ifndef SHARED_MARSLANDER_STATE_H_
#define SHARED_MARSLANDER_STATE_H_

#include "common.h"

#include <cstdint>
#include <vector>

namespace marslander {

struct state final
    : public game_init_input,
      public game_turn_input {

  span<surface_type::value_type::value_type> safe_area_x;
  surface_type::value_type::value_type safe_area_alt;

  game_turn_output out;

  void from_base64(const std::string&);
  std::string to_base64() const;

};

} // namespace marslander

#endif // SHARED_MARSLANDER_STATE_H_
