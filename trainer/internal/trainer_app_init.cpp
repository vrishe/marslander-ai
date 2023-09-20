#include "trainer_app.h"
#include "trainer_input.h"
#include "internal/trainer_data.h"

#include <chrono>
#include <exception>
#include <future>
#include <limits>
#include <random>
#include <string>
#include <thread>

namespace marslander::trainer {

using namespace std;
using namespace nlohmann;

void app::do_init() {

  bool will_export = _args.export_dump_session_flag
    || _args.export_replay_flag;

  bool init_from_scratch = _args.init_flag;
  {
    ifstream training(get_data_path(training_filename), ios::binary);

    if (training) {
      if (init_from_scratch) {
        cout << "There's a '" << training_filename 
                << "' file found in the working directory!\n"
  "Proceeding with initialization will result in OVERWRITING it.\n"
  "Would you like to begin training process over entirely (y/[N])? ";

        init_from_scratch = input::wait_answer();
      }

      if (!init_from_scratch)
        do_init_from_file(move(training));
    }
    else {
      if (will_export && !init_from_scratch
          || _args.export_replay_flag) {
        cerr << "No '" << training_filename
                << "' file found. There's no session to export data from!";

        exit(-2);
      }

      init_from_scratch = true;
    }
  }

  if (init_from_scratch)
    do_init_from_scratch();

  if (will_export) {
    if (_args.export_replay_flag)       do_make_replay();
    if (_args.export_dump_session_flag) do_dump_session();

    if (!_args.no_exit_flag) exit(_last_error);
  }

  on_server_initialized();
}

namespace {

#include "trainer_app_init_algo_fac.icc"

string state_digest(const app_state& s) {
  return fmt::format(
" check:           {}\n"
" generation:      {}\n"
" cases count:     {}\n"
" population size: {}\n"
" elite count:     {}\n"
" tournament size: {}\n"
" crossover:       {}\n"
" mutation:        {}",
    s.check,
    s.generation,
    s.cases_count,
    s.population_size,
    s.elite_count,
    s.tournament_size,
    s.crossover,
    s.mutation);
}

} // namespace

template<class CharT, class Traits>
basic_ostream<CharT, Traits>& operator<<(basic_ostream<CharT, Traits>& dst,
    const algorithm_args& args) {

  dst << quoted(args.name, '\'');
  if (!args.values.empty()) {
    dst << " " << std::fixed << args.values[0];
    for (size_t i = 1, imax = args.values.size(); i < imax; ++i) 
      dst << ", " << std::fixed << args.values[i];
  }

  return dst;
}

void app::do_init_from_file(ifstream&& training) {

  if (!check_integrity(training)) {
    cerr << training_filename << " is corrupted." << endl;
    exit(-3);
  }

  auto& s = _state;
  training.exceptions(ios_base::badbit | ios_base::failbit);
  s.read(training, app_state::header);
  training.exceptions(ios_base::goodbit);

  if (!xvr_factory().instantiate(s.crossover, s.prng, s.pxvr)){
    cerr << quoted(s.crossover.name, '\'') 
      << " unrecognized crossover algorithm; "
         "is it no longer supported?\n"
      << s.crossover << endl;

    exit(-3);
  }
  if (!mtn_factory().instantiate(s.mutation, s.prng, s.pmtn)) {
    cerr << quoted(s.mutation.name, '\'') 
      << " unrecognized mutation algorithm; "
         "is it no longer supported?\n"
      << s.mutation << endl;

    exit(-3);
  }

  _state_future = async(launch::async,
    [
      &s,
      is = move(training)
    ]
    () mutable {
      s.read(is, app_state::body);

      cout << "Recovered training state!\n"
        << state_digest(s) << endl;
      on_generation_changed(s);
    });
}

namespace {

using predefined_cases_t = vector<data::landing_case>;
inline predefined_cases_t read_cases_file(
    const filesystem::path& path) {

  json j = json::parse(ifstream{path});
  return j.get<vector<data::landing_case>>();
}

inline auto read_cases_file_async(const filesystem::path& path) {
  if (path.empty()) {
    return async(launch::deferred,
      []() { return vector<data::landing_case>(); });
  }

  return async(launch::async, read_cases_file, path);
}

constexpr auto dbl_max_ = numeric_limits<double>::max();
constexpr auto ul_max_ = numeric_limits<unsigned long>::max();

template<class Factory, class PRNG>
static void read_algo(string title, algorithm_args& in_out_args, PRNG prng,
    typename Factory::result_type& result) {
  using namespace marslander::input;
  for(Factory f;;) {
    read_input(title,
      "Enter <alg name>[; <value>[; <value>]]",
      &cvt_alg_args, in_out_args);

    typename Factory::errors_t errors;
    if (f.validate_args(in_out_args, errors)) {
      f.instantiate(in_out_args, prng, result);
      break;
    }

    for (auto& e : errors)
      cerr << in_out_args.name << ": " << e << endl;
    cerr << endl;
  }
}

void setup_population(app_state& s) {
  using namespace marslander::input;
  using namespace std::placeholders;

  read_input("Population size [1]:",
    "Enter a positive number greater than or equal 1.",
    bind(&cvt_num_ul, _1, _2, 1, ul_max_, 1), s.population_size);

  for(;;) {
    unit_value elite_units{};
    read_input(
      "Elite individuals count [0]:",
      fmt::format("Enter percentage or a non-negative number "
          "less than or equal {}.", s.population_size),
      bind(&cvt_unit, _1, _2, s.population_size, 0), elite_units);
    s.elite_count = elite_units(s.population_size);
    if (elite_units.unit != units::amount)
      cout << s.elite_count << " elite individuals." << endl;

    if (s.elite_count > 0 
        && s.elite_count >= unit_value::percent(5)(s.population_size)) {
      cout << "Elite count of choice may be SUBOPTIMAL. "
        "Do you want to keep this value anyway (y/[N])? ";

      if (!wait_answer()) continue;
    }

    break;
  }

  bool has_crossover = true;
  auto tournament_size_max = s.population_size - s.elite_count;
  if (tournament_size_max > 1) {
    read_input("Selection tournament size [1]:",
      fmt::format("Enter a number in range [1; {}].", tournament_size_max),
      bind(&cvt_num_ul, _1, _2, 1, tournament_size_max, 1), s.tournament_size);
  }
  else if (tournament_size_max == 1) {
    s.tournament_size = tournament_size_max;
    cout << "Tournament size is: " << tournament_size_max << endl;
  }
  else {
    cout << "Crossover pass is skipped." << endl;
    has_crossover = false;
  }

  if (has_crossover) read_algo<xvr_factory>("Crossover:",
    s.crossover, s.prng, s.pxvr);
  else s.crossover = algorithm_args{};

  read_algo<mtn_factory>("Mutation:", s.mutation, s.prng, s.pmtn);
}

void setup_cases(app_state& s, size_t predefined_cases_size) {
  using namespace marslander::input;
  using namespace std::placeholders;

  if (predefined_cases_size > 0) { 
    cout << "There are " << predefined_cases_size
      << " pre-defined training cases available!" << endl;
  }

  s.cases_count = max(predefined_cases_size, size_t(1));
  read_input(fmt::format("Training cases count [{}]:", s.cases_count),
    "Enter a positive number greater than or equal 1.",
    bind(&cvt_num_ul, _1, _2, 1, ul_max_, s.cases_count), s.cases_count);

  if (s.cases_count > predefined_cases_size) {
    cout << (s.cases_count - predefined_cases_size)
      << " cases will be generated randomly." << endl;
  }
}

} // namespace

void app::do_init_from_scratch() {
  auto cases_task = read_cases_file_async(_args.cases_path);

  auto& s = _state;
  setup_population(s);

  predefined_cases_t predefined_cases{};
  try { predefined_cases = cases_task.get(); }
  catch (exception& e) { SPDLOG_LOGGER_WARN(_logger, e.what()); }
  setup_cases(s, predefined_cases.size());

  s.generation = 0;
  s.check = chrono::time_point_cast<chrono::seconds>(
      chrono::system_clock::now()).time_since_epoch().count();

  using namespace marslander::input;
  using namespace std::placeholders;

  auto sway = genome_sway;
  read_input(fmt::format("Genome randomizer sway [{}]:", sway),
    "Enter a positive number.",
    bind(&cvt_num_dbl, _1, _2, 0, dbl_max_, sway), sway);

  _state_future = async(launch::async,
    [
      &s,
      sway,
      predefined_cases = move(predefined_cases)
    ]
    () mutable {
      auto population_task = async(launch::async,
        [&s, sway] () mutable {
          s.population.resize(s.population_size);

          ITERZ_(s.population, dst);
          while(!ITERZ_END(dst)) {
            auto& item = *dst_it++;
            item.set_id(s.uids.next_uid());
            // nn::randomize<nn::activators::ReLU>(*s.prng, item);
            nn::randomize_naive(*s.prng, item, sway);
          }
        });

      s.cases.resize(s.cases_count);

      ITERZ_(s.cases, dst);
      ITERZ_(predefined_cases, src);
      while(!ITERZ_END(dst) && !ITERZ_END(src)) {
        auto& item = *dst_it++;
        data::convert(*src_it++, item);
        item.set_id(s.uids.next_uid());
      }

      if (!ITERZ_END(dst)) {
        do {
          auto& item = *dst_it++;
          landing_case::randomize(*s.prng, item);
          item.set_id(s.uids.next_uid());
        }
        while (!ITERZ_END(dst));
      }

      population_task.wait();
      {
        [[maybe_unused]]
        auto _ = async(launch::async,
          [&s] () mutable { persist_state(s); });

        cout << "Initialized training state!\n"
          << state_digest(s) << endl;
        on_generation_changed(s);
      }
    });
}

} // namespace marslander::trainer
