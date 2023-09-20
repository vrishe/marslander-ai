#include "internal/client.h"
#include "internal/replay_exporter.h"
#include "marslander/state.h"
#include "global_includes.h"

#include "nn.h"

#include "sockpp/platform.h"

#include <filesystem>
#include <utility>
#include <vector>

namespace marslander::runner {

struct app_args final {
  in_addr_t host;
  in_port_t port;

  std::filesystem::path replays_dir;
  size_t replays_count;
};

struct app_state final {

  using brain_t = nn::DFF<fnum>;

  std::vector<std::pair<uid_t, state>> states;
  std::vector<std::pair<uid_t, brain_t>> population;

  uint64_t check;
  size_t capacity_base;
  pb::outcomes req;

  std::unique_ptr<basic_replay_exporter> pexp;
};

class app final {

  static logger_ptr _logger;

  const app_args _args;

  app_state _state;
  app_state& state() { return _state; }

  void do_init();

  void do_simulation();

public:

  app();

  template<class Init>
  void init_args(Init&& init) {
    init(const_cast<app_args&>(_args));
  }

  void run();

};

} // namespace marslander::runner
