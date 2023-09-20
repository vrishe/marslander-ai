#include "runner_app.h"
#include "marslander/marslander.h"

#include <algorithm>
#include <chrono>
#include <iterator>
#include <ostream>

template<typename CharT, typename Traits>
static std::basic_ostream<CharT, Traits>& marslander::operator<<(
    std::basic_ostream<CharT, Traits>& os, outcome o) {
  switch (o) {
    case outcome::Aerial:  return os << "Aerial";
    case outcome::Landed:  return os << "Landed";
    case outcome::Crashed: return os << "Crashed";
    case outcome::Lost:    return os << "Lost";
  }
  return os << "Unknown [" << int(o) << "]";
}

namespace marslander::runner {

using namespace std;
using namespace std::chrono_literals;

namespace {

double eval_outcome_rating(size_t steps, outcome o, const state& s,
    const game_init_input& game_init, const game_turn_input& turn_zero) {
  switch(o) {
    case outcome::Landed: {
      auto safe_width  = .5 * (s.safe_area_x.end - s.safe_area_x.start);
      auto safe_center = s.safe_area_x.start + safe_width;
      return   10. * (steps / double(steps_limit))
             + 60. * (1 - s.fuel / double(turn_zero.fuel))
             + 30. * (abs(s.position.x - safe_center) / safe_width);
    }
    case outcome::Crashed: {
      auto safe_center = .5 * (s.safe_area_x.start + s.safe_area_x.end);
      return 100. + 20. * (steps / double(steps_limit))
                  + 20. * (1 - s.fuel / double(turn_zero.fuel))
                  + 35. * (abs(s.position.x - safe_center)
                      / constants::zone_width)
                  + 25. * (abs(s.position.y - s.safe_area_alt)
                      / double(turn_zero.position.y - s.safe_area_alt));
    }
    default:
      return 200. + 100. * (steps / double(steps_limit));
  }
}

} // namespace

void app::do_simulation() {
  auto& s = state();

  for(client::response r;;) {
    try { r = client::request({_args.host, _args.port}, s.req); }
    catch (std::logic_error& e) { CLIENT_LOOP_REP_(1, e.what()); }
    catch (io::transfer_error& e) {
      SPDLOG_LOGGER_WARN(_logger, "Simulation process interrupted: {}",
        e.what());
      looper::current().post(&app::do_init, this);
      return;
    }

    auto population = r.begin()->as<pb::population>();
    s.population.clear();
    s.req.clear_data();
    s.req.set_generation(population->generation());

    auto& population_data = population->data();
    CLIENT_LOOP_REP_(population_data.empty(), "No population's been given.");

    using converter = data::convert_f<app_state::brain_t::value_type>;
    transform(population_data.begin(), population_data.end(),
      back_inserter(s.population), converter());

    SPDLOG_LOGGER_TRACE(_logger, "Received population of {} individuals.",
      s.population.size());
    break;
  }

  using clk_t = chrono::steady_clock;
  auto start = clk_t::now();
  {
    auto outcomes = s.req.mutable_data();
    outcomes->Clear();

    for (const auto& sp : s.population)
    for (const auto& ss : s.states) {
      auto sim_state{ss.second};
      nn::game_adapter a(sp.second, ss.second, ss.second);

      // SPDLOG_LOGGER_TRACE(_logger, "Running gene #{} at case #{}",
      //   sp.first, ss.first);

      s.pexp->reset(sim_state);

      auto steps = steps_limit;
      auto o = outcome::Aerial;
      for (;o == outcome::Aerial & steps > 0; --steps) {
        sim_state.out = a.get_output(sim_state);
        o = simulate(sim_state);

        s.pexp->push_turn(sim_state);
      }

      auto outcome = outcomes->Add();
      outcome->set_case_id(ss.first);
      outcome->set_genome_id(sp.first);
      outcome->set_rating(eval_outcome_rating(
        steps, o, sim_state, ss.second, ss.second));

      if (o == outcome::Landed)
        SPDLOG_LOGGER_INFO(_logger, "#{} {}! {}@{}\n"
          " scr: {}\n"
          " pos: {}\n"
          " vel: {}\n"
          " tlt: {}",
          s.req.generation(), o, sp.first, ss.first,
          outcome->rating(),
          sim_state.position,
          sim_state.velocity,
          sim_state.tilt);

      s.pexp->do_export(s.req.generation(),
        ss.first, sp.first, o);
    }
  }
  auto duration = clk_t::now() - start;
  SPDLOG_LOGGER_TRACE(_logger, "Processed {} individuals @ {} cases for {}.",
    s.population.size(), s.states.size(), duration);

  auto new_capacity = s.capacity_base * (300ms / duration);
  s.req.set_capacity(max(new_capacity, s.capacity_base));
  SPDLOG_LOGGER_TRACE(_logger, "New capacity is {}.", new_capacity);

  looper::current().post(&app::do_simulation, this);
}

} // namespace marslander::runner
