#include "trainer_app.h"

#include <stdexcept>

namespace marslander::trainer {

using namespace std;

app::app() : _args{} {}

logger_ptr app::_logger;

void app::run() {
  _logger = spdlog::get(loggers::trainer_logger);

  looper l{};
  l.post(&app::do_init, this);
  l.run();
}

app_state& app::state() {
  if (_state_future.valid())
    _state_future.get();

  return _state;
}

std::filesystem::path app::get_data_path() const {
  return _args.directory;
}

std::filesystem::path app::get_data_path(
    const std::filesystem::path& p) const {
  if (!p.is_relative()) throw std::logic_error(
    fmt::format("not a relative path: {}", p));
  return _args.directory / p;
}

} // namespace marslander::trainer
