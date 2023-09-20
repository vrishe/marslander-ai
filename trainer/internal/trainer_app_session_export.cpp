#include "trainer_app.h"
#include "trainer_input.h"
#include "internal/trainer_data.h"
#include "marslander/marslander.h"

#include "common.h"
#include "nn.h"

#include "nlohmann/json.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace marslander::trainer {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(algorithm_args,
  name, values);

using namespace std;
using namespace nlohmann;

constexpr inline auto id_max_ = numeric_limits<uid_t>::max();

void app_args::parse_replay_optarg(const std::string& optarg,
    const std::string& delim) {
  using namespace marslander::input;
  auto in = split(string(optarg), delim, true);
  if (in.size() > 0) cvt_num_ul(in[0], replay_gene_id,
      1, id_max_, replay_gene_id);
  if (in.size() > 1) cvt_num_ul(in[1], replay_case_id,
      1, id_max_, replay_case_id);
}

void app::do_dump_session() {
  auto& s = state();

  auto pop_sz = s.population.size();
  constexpr auto size_threshold = 1000;
  while (pop_sz > size_threshold) {
    cout << "Population output gotta be HUGE ("
      << pop_sz << " entries)!\n"
  "Would you like to specify a number of population "
  "entries to sample ([Y]/n)? ";

    using namespace marslander::input;
    using namespace std::placeholders;

    if (!wait_answer(answers::yes_y)) break;

    pop_sz = s.population.size();
    unit_value sample_units{};
    read_input(
      fmt::format("Population samples count [{}]:",
          size_threshold),
      fmt::format("Enter percentage or a non-negative number "
          "less than or equal {}.", pop_sz),
      bind(&cvt_unit, _1, _2, pop_sz, size_threshold), sample_units);
    pop_sz = sample_units(pop_sz);
    if (sample_units.unit != units::amount)
      cout << pop_sz << " population entries." << endl;
  }

  std::vector<data::landing_case> json_cases(s.cases.size());
  for (size_t i = 0, imax = json_cases.size(); i < imax; ++i)
    convert_back(s.cases[i], json_cases[i]);

  std::vector<data::genome> json_population(pop_sz);
  if (pop_sz < s.population.size()) {
    vector<size_t> inds; inds.reserve(pop_sz);
    uniform_int_distribution<size_t> d{0, s.population.size()-1};
    while (inds.size() < pop_sz) {
      auto i = d(*s.prng);
      auto inds_it = lower_bound(inds.begin(), inds.end(), i);
      if (inds_it == inds.end() || i < *inds_it)
        inds.insert(inds_it, i);
    }

    size_t j = 0;
    for (auto i : inds)
      convert_back(s.population[i], json_population[j++]);
  }
  else {
    for (size_t i = 0, imax = json_population.size(); i < imax; ++i)
      convert_back(s.population[i], json_population[i]);
  }

  bool standard_out = true;
  std::ostream* dst = &std::cout;
  auto file_path = _args.dump_session_path;
  if (!file_path.empty() && file_path.is_relative())
    file_path = get_data_path(file_path);
  ofstream dstf_(file_path);
  if (dstf_) {
    standard_out = false;
    dst = &dstf_;
  }
  else if (!_args.dump_session_path.empty()) {
    cerr << "Can't write a session dump into " << _args.dump_session_path
      << endl;
    _last_error = -2;
    return;
  }

  if (standard_out) (*dst) << PRETTY_JSON;
  (*dst) << JSON_SESSION_DUMP(
    s.check,
    s.generation,
    s.cases_count,
    s.population_size,
    s.elite_count,
    s.tournament_size,
    s.crossover,
    s.mutation,
    json_cases,
    json_population);

  if (standard_out) cout << endl;
  cout << "Done dumping the session." << endl;
}
 
void app::do_make_replay() {
  auto& s = state();

  uid_t case_id = _args.replay_case_id,
        gene_id = _args.replay_gene_id;

  bool may_swap_ids = true;

  using namespace marslander::input;
  using namespace std::placeholders;

  if (gene_id <= 0) {
    may_swap_ids = false;
    read_input("Enter Genome ID [1]: ", "ID is a non-negative value.",
      bind(cvt_num_ul, _1, _2, 1, id_max_, 1), gene_id);
  }
  else cout << "Genome ID: " << gene_id << endl;

  if (case_id <= 0) {
    may_swap_ids = false;
    read_input("Enter Case ID [1]: ", "ID is a non-negative value.",
      bind(cvt_num_ul, _1, _2, 1, id_max_, 1), case_id);
  }
  else cout << "Case ID:   " << case_id << endl;

  auto case_ind = s.cases_index.find(case_id);
  if (may_swap_ids && case_ind == s.cases_index.end()) {
    cout << "Hmm, Case with ID " << case_id << " is not found;"
  " Let us swap incoming IDs and try again." << endl << endl;

    swap(gene_id, case_id);
    cout << "Genome ID: " << gene_id << endl;
    cout << "Case ID:   " << case_id << endl;

    case_ind = s.cases_index.find(case_id);
  }
  if (case_ind == s.cases_index.end()) {
    cerr << "There is no Case with ID " << _args.replay_case_id << endl;
    _last_error = -1;
    return;
  }

  auto population_ind = s.population_index.find(gene_id);
  if (population_ind == s.population_index.end()){
    cerr << "There is no Genome with ID " << gene_id << endl;
    _last_error = -1;
    return;
  }

  using brain_t = nn::DFF<fnum>;

  auto sim_state{move(data::convert(*case_ind->second).second)};
  auto brain{move(data::convert_f<brain_t::value_type>{}(*population_ind->second).second)};
  nn::game_adapter a(brain, sim_state, sim_state);

  using turns_t = std::vector<game_turn_input>;
  turns_t turns;
  turns.reserve(steps_limit);
  turns.push_back(sim_state);

  auto sim_state_base64 = sim_state.to_base64();

  auto steps = steps_limit;
  auto o = outcome::Aerial;
  for (;o == outcome::Aerial & steps > 0; --steps) {
    sim_state.out = a.get_output(sim_state);
    o = simulate(sim_state);

    turns.push_back(sim_state);
  }

  filesystem::path file_path;
  {
    const time_t t_c = chrono::system_clock::to_time_t(
      chrono::system_clock::now());
    file_path = get_data_path((stringstream()
        << "replay_" << s.generation << "_" << gene_id << "_" << case_id
        << "_" << put_time(localtime(&t_c), "%F_%H-%M-%S") << ".json").str());
  }

  ofstream dstf_(file_path);
  if (!dstf_) {
    cerr << "Can't write a replay into " << file_path.c_str() << endl;
    _last_error = -2;
    return;
  }

  dstf_ << PRETTY_JSON
  << JSON_REPLAY(
    case_id,
    gene_id,
    o,
    sim_state_base64,
    forward<game_init_input>(sim_state),
    turns);

  cout << "Done exporting the replay." << endl;
}

} // namespace marslander::trainer
