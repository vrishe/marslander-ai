#pragma once

#ifndef SHARED_MARSLANDER_SIMULATION_H_
#define SHARED_MARSLANDER_SIMULATION_H_

#include "./state.h"

namespace marslander {

constexpr inline size_t steps_limit = 256;

enum class outcome { Aerial = -1, Landed, Crashed, Lost };

outcome simulate(state& state);

} // namespace marslander

#endif // SHARED_MARSLANDER_SIMULATION_H_
