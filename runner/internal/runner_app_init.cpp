#include "runner_app.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iterator>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <thread>

namespace marslander::runner {

using namespace std;

void app::do_init() {
  auto& s = state();

  s.capacity_base = 16;
  s.req.clear_data();
  s.req.set_capacity(s.capacity_base);
  s.req.set_client_name(loggers::runner_logger);

  if (_args.replays_count > 0) {
    cout << "Runner is configured to keep at most " << _args.replays_count
      << " replay(s) at " << quoted(_args.replays_dir.c_str()) << endl;
    if (filesystem::is_directory(_args.replays_dir)
        && !filesystem::is_empty(_args.replays_dir)) {
      cout << "Replays directory " << quoted(_args.replays_dir.c_str())
        << " is not empty;\nREMOVE all the contents AND PROCEED (y/[N])? ";

      if (input::wait_answer()) {
        auto n = filesystem::remove_all(_args.replays_dir);
        cout << "Deleted " << n << " files or directories." << endl;
      }
    }

    s.pexp = make_unique<replay_exporter>(_logger,
      _args.replays_dir, _args.replays_count);
  }
  else {
    s.pexp = make_unique<basic_replay_exporter>();
  }

  SPDLOG_LOGGER_INFO(_logger, "Ready!", s.req.client_name());

  for(client::response r;;) {
    try { r = client::request<pb::cases>({_args.host, _args.port}); }
    catch (std::exception &e) { CLIENT_LOOP_REP_(1, e.what()); }

    auto& cases = r.begin()->as<pb::cases>()->data();
    CLIENT_LOOP_REP_(cases.empty(), "No cases obtained.");

    s.states.clear();
    s.states.reserve(cases.size());
    transform(cases.begin(), cases.end(),
      back_inserter(s.states), data::convert);

    SPDLOG_LOGGER_TRACE(_logger, "Received {} cases.", s.states.size());
    break;
  }

  looper::current().post(&app::do_simulation, this);
}

} // namespace marslander::runner
