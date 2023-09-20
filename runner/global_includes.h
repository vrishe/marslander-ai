#pragma once

#ifndef RUNNER_GLOBAL_INCLUDES_H_
#define RUNNER_GLOBAL_INCLUDES_H_

#include "shared.h"

#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

#include <string>

namespace marslander {

using logger_ptr = std::shared_ptr<spdlog::logger>;

namespace loggers {

constexpr inline char client_logger[] = "client";
extern const std::string runner_logger;

} // namespace loggers

} // namespace marslander

#endif // RUNNER_GLOBAL_INCLUDES_H_
