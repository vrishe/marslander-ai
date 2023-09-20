#include "internal/concurrent_random_engine.h"
#include "internal/ga.h"
#include "internal/results_table.h"
#include "internal/server.h"
#include "global_includes.h"

#include "sockpp/platform.h"

#include <filesystem>
#include <future>
#include <map>
#include <ostream>
#include <random>
#include <string>
#include <vector>

namespace marslander::trainer {

struct app_args final {
  in_port_t port;

  int init_flag;
  std::filesystem::path cases_path;

  int export_dump_session_flag;
  std::filesystem::path dump_session_path;

  int export_replay_flag;
  uid_t replay_case_id, replay_gene_id;

  int no_exit_flag;

  std::filesystem::path directory;

  void parse_replay_optarg(const std::string& optarg,
    const std::string& delim);
};

struct algorithm_args final {
  using value_type = double;

  std::string name;
  std::vector<value_type> values;
};

template<typename T>
struct index_entry {

  using ind_t = typename std::vector<T>::size_type;
  using ref_t = typename std::vector<T>::reference;

  index_entry(ind_t index, ref_t element)
    : _index{index},
      _elem{element}
    {}

  index_entry(const index_entry& ) = default;
  index_entry(      index_entry&&) = default;

  ind_t index() const { return _index; }
  operator ind_t() const { return _index; }

  ref_t operator*() {return _elem; }

private:

  const ind_t _index;
  const ref_t _elem;

};

struct app_state final {

  uint64_t check;

  size_t generation, cases_count, elite_count,
         population_size, tournament_size;

  algorithm_args crossover, mutation;

  uid_source uids;

  using cases_t = std::vector<pb::landing_case>;
  cases_t cases;

  using population_t = std::vector<pb::genome>;
  population_t population;

  // Add serialized data above this line

  using random_engine_t = concurrent_random_engine<std::mt19937_64, 4096>;
  std::shared_ptr<random_engine_t> prng = std::make_shared<random_engine_t>(
    std::move(std::mt19937_64{std::random_device{}()}));

  std::unique_ptr<crossover_algo> pxvr;
  std::unique_ptr<mutation_algo> pmtn;

  enum read_mode {
    header = 1,
    body = 2,
    all = 3
  };

  std::istream& read (std::istream&, read_mode = all);
  std::ostream& write(std::ostream&) const;

  template<typename T>
  using index_t = std::map<uid_t, index_entry<T>>;
  index_t<pb::landing_case> cases_index;
  index_t<pb::genome> population_index;
  void rebuild_indices();

  size_t index;
  using timeout_clock_t = std::chrono::steady_clock;
  std::vector<timeout_clock_t::time_point> timeouts;
  results_table results;
  void reset_results();

};

#define REQUEST_HANDLER_MEM_DECL_(msg)\
  on_ ## msg ## _request(server::request<pb::msg>&& request,\
    server::response_sink& response)

class app final {

  static constexpr char training_filename[] = "training.dat";
  static logger_ptr _logger;

  const app_args _args;

  app_state _state;
  std::future<void> _state_future;
  app_state& state();

  static std::ifstream& check_integrity(std::ifstream&);

  static void on_generation_changed(app_state&);

  static void persist_state(const app_state&);

  std::filesystem::path get_data_path() const;
  std::filesystem::path get_data_path(
    const std::filesystem::path&) const;

  void do_init();

  void do_init_from_file(std::ifstream&&);

  void do_init_from_scratch();

  int _last_error;
  void do_dump_session();
  void do_make_replay();

  void on_server_initialized();

  // Request handlers follow
  void REQUEST_HANDLER_MEM_DECL_(cases);
  void REQUEST_HANDLER_MEM_DECL_(outcomes);

public:

  app();

  template<class Init>
  void init_args(Init&& init) {
    init(const_cast<app_args&>(_args));
  }

  void run();

};

} // namespace marslander::trainer
