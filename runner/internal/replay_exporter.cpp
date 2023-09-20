#include "replay_exporter.h"
#include "global_includes.h"
#include "marslander/marslander.h"

#include "common.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <future>
#include <iterator>
#include <ostream>
#include <regex>
#include <stdexcept>

namespace marslander::runner {

using namespace std;
using namespace nlohmann;

namespace {

void remove_excessive_files(logger_ptr logger,
    const std::filesystem::path& dir, size_t max_count) {
  using namespace filesystem;

  error_code ec;
  size_t remove_count;

  vector<directory_entry> entries;
  copy_if(directory_iterator(dir, ec), directory_iterator(),
    back_inserter(entries),
    [](const auto& entry) {
      return entry.is_regular_file();
    });

  if (ec) {
    SPDLOG_LOGGER_TRACE(logger, "Failed to enumerate directory '{}' ({}).",
      dir, ec);

    goto bail_error;
  }

  remove_count = entries.size();
  if (remove_count < max_count) return;

  sort(entries.begin(), entries.end(),
    [](const auto& a, const auto& b) { 
      return a.last_write_time() < b.last_write_time();
    });

  for (const auto& p : entries) {
    if (filesystem::remove(p, ec) && --remove_count < max_count) break;
    SPDLOG_LOGGER_WARN(logger, "Failed to remove file '{}' ({}).",
      p, ec);
  }

  if (ec) {
bail_error:
    auto e = std::runtime_error("Failed to clean up replays directory.");
    SPDLOG_LOGGER_ERROR(logger, e.what());
    throw e;
  }
}

} // namespace

void replay_exporter::do_export(size_t generation, uid_t gid, uid_t cid,
    outcome o) {

  if (o != outcome::Landed || ++_times != _each) {
    _turns.clear();
    return;
  }

  _times = 0;
  _save = async(launch::async,
    [
      generation, gid, cid, o,
      dir = _dir,
      logger = _logger,
      max_count = _max_count,
      state = move(_state),
      turns = move(_turns),
      last_save = move(_save)
    ]
    () mutable {
      error_code ec;
      if (!filesystem::create_directories(dir, ec) && ec) {
        SPDLOG_LOGGER_ERROR(logger,
          "Can't create replays directory '{}'", dir);
        return;
      }

      stringstream filename;
      {
        const time_t t_c = chrono::system_clock::to_time_t(
          chrono::system_clock::now());
        filename << "replay_" << generation << "_"
                                    << gid << "_" << cid << "_"
          << put_time(localtime(&t_c), "%F_%H-%M-%S") << ".json";
      }

      if (last_save.valid()) last_save.wait();
      remove_excessive_files(logger, dir, max_count);
      auto file_path = dir / filename.str();
      {
        ofstream dstf_(file_path);
        if (!dstf_) {
          SPDLOG_LOGGER_ERROR(logger,
            "Can't write a replay into '{}'", file_path);
          return;
        }

        game_init_input& init = state;
        dstf_ << PRETTY_JSON
        << JSON_REPLAY(cid, gid, o, state.to_base64(), init, turns);
      }
      SPDLOG_LOGGER_TRACE(logger, "Saved replay '{}'.", file_path);
    });
}

void replay_exporter::push_turn(const game_turn_input& turn) {
  _turns.push_back(turn);
}

void replay_exporter::reset(const state& state) {
  _state = state;
  _turns.clear();
  _turns.push_back(state);
}

} // namespace marslander::runner
