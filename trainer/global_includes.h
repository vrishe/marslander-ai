#pragma once

#ifndef TRAINER_GLOBAL_INCLUDES_H_
#define TRAINER_GLOBAL_INCLUDES_H_

#include "shared.h"

// #define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
#    define DEBUG_(text) text
#else
#    define DEBUG_(text)
#endif

namespace marslander {

using logger_ptr = std::shared_ptr<spdlog::logger>;

namespace loggers {

constexpr inline char server_logger[]  = "server";
constexpr inline char trainer_logger[] = "trainer";

} // namespace loggers

} // namespace marslander

#endif // TRAINER_GLOBAL_INCLUDES_H_
