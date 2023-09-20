#include "global_includes.h"
#include "marslander/marslander.h"

#include "common.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <iterator>
#include <ostream>

namespace marslander::runner {

class basic_replay_exporter {

public:

  basic_replay_exporter() {}
  virtual ~basic_replay_exporter() = default;

  virtual void do_export(size_t, uid_t, uid_t, outcome) {}
  virtual void push_turn(const game_turn_input&) {}
  virtual void reset(const state&) {}

};

class replay_exporter final : public basic_replay_exporter {

  using turns_t = std::vector<game_turn_input>;

  const std::filesystem::path _dir;
  const size_t _max_count, _each;

  logger_ptr _logger;
  size_t _times;

  std::future<void> _save;
  state _state;
  turns_t _turns;

public:

  replay_exporter(logger_ptr logger,
        const std::filesystem::path& dir, size_t max_count, size_t each = 1)
    : _dir{dir},
      _max_count{max_count},
      _each{each},
      _logger{logger},
      _times{}
    {}

  void do_export(size_t, uid_t, uid_t, outcome) override;
  void push_turn(const game_turn_input&) override;
  void reset(const state&) override;

};

} // namespace marslander::runner
