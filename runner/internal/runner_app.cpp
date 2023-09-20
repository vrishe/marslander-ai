#include "runner_app.h"

#include "global_includes.h"

namespace marslander::runner {

using namespace std;

app::app() : _args{} {}

logger_ptr app::_logger;

void app::run() {
  _logger = spdlog::get(loggers::runner_logger);

  looper l{};
  l.post(&app::do_init, this);
  l.run();
}

} // namespace marslander::runner
