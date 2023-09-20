#include "trainer_app.h"

namespace marslander::trainer {

void app_state::rebuild_indices() {

  DEBUG_(
    auto logger = spdlog::get(loggers::trainer_logger);
    )

  cases_index.clear();
  for (auto& item : cases) {
    DEBUG_(
      auto result =
      )
    cases_index.insert({item.id(), {cases_index.size(), item}});

    DEBUG_(
        if (!result.second)
          logger->debug("Cases index conflict! ID: {} ", item.id());
      )
  }

  population_index.clear();
  for (auto& item : population) {
    DEBUG_(
      auto result =
      )
    population_index.insert({item.id(), {population_index.size(), item}});

    DEBUG_(
        if (!result.second)
          logger->debug("Population index conflict! ID: {} ", item.id());
      )
  }
}

} // namespace marslander::trainer
