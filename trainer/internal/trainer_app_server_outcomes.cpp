#include "trainer_app.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <execution>
#include <iterator>
#include <limits>
#include <random>

namespace marslander::trainer {

using namespace std;

constexpr inline auto results_timeout = 30s;

void app_state::reset_results() {
  index = 0;
  timeouts.clear();
  constexpr auto default_timeout = timeout_clock_t::time_point() - results_timeout;
  timeouts.resize(population.size(), default_timeout);
  timeouts.shrink_to_fit();
  results.resize(cases.size(), population.size(),
      std::numeric_limits<decltype(results)::value_type>::quiet_NaN());
}

void app::on_generation_changed(app_state& s) {
  s.rebuild_indices();
  s.reset_results();

  SPDLOG_LOGGER_TRACE(_logger, "==== GENERATION {} ====", s.generation);
}

namespace {

struct generation_stats {
  size_t generation;
  score_t score_best, score_worst;
};

class xvr_tournament final {
  uniform_int_distribution<size_t> _d;
public:
  xvr_tournament(size_t ofs, size_t total) : _d{ofs, total - 1} {}
  template<class Rng>
  size_t operator()(Rng&& rng, size_t size) {
    auto result = ~size_t{};
    while (size--) {
      auto v = _d(rng);
      if (v < result) result = v;
    }
    return result;
  }
};

class xvr_iterator final : public child_output_query<pb::genome> {
  app_state::population_t::iterator _it;
  size_t _n;
  const size_t _step;

public:
  using difference_type = app_state::population_t::iterator::difference_type;
  using value_type = child_output_query<pb::genome>;
  using pointer = void;
  using reference = value_type&;
  using iterator_category = forward_iterator_tag;

  xvr_iterator(app_state::population_t& p)
    : _it{p.end()}, _n{}, _step{} {}
  xvr_iterator(app_state::population_t& p, size_t step, size_t offset = 0)
    : _it{std::next(p.begin(), offset)}, _n{step}, _step{step} { }

  xvr_iterator(const xvr_iterator& ) = default;
  xvr_iterator(      xvr_iterator&&) = default;
  xvr_iterator& operator=(const xvr_iterator& ) = default;
  xvr_iterator& operator=(      xvr_iterator&&) = default;

  bool operator!=(const xvr_iterator& other) const
  { return _it != other._it; }
  bool operator==(const xvr_iterator& other) const
  { return _it == other._it || !_step && !other._step; }
  xvr_iterator& operator++()
  { std::advance(_it, _n); _n = _step; return *this; }
  xvr_iterator operator++(int)
  { auto ret{*this}; std::advance(_it, _n); _n = _step; return ret; }
  reference operator*() {return *this;}

  pb::genome& next_child() override {
    --_n; assert(_n <= _step);
    return *_it++;
  }
};

#define ALIGN_(v, n) ((((v) + (n - 1)) / (n)) * (n))

void next_generation(app_state& state, generation_stats& out_stats) {
  vector<size_t> inds(state.population.size());
  {
    vector<score_t> score(state.population.size());
    transform(execution::par_unseq,
      state.results.begin(), state.results.end(), score.begin(),
      [d = state.results.cols()](auto&& row_p) {
        // TODO: score calculation may be a subject for change.
        return fp::fast_sum(get<1>(row_p), get<2>(row_p)) / d;
      });

    iota(inds.begin(), inds.end(), 0);
    sort(inds.begin(), inds.end(), [&score](auto u, auto v) {
      return score[u] < score[v];
    });

    out_stats.generation = state.generation;
    out_stats.score_best = score[inds.front()];
    out_stats.score_worst = score[inds.back()];
  }

  app_state::population_t new_pop;

  auto xvr_growth = state.pxvr->meta().growth;
  auto pop_elite_count = min(state.elite_count, state.population_size);
  auto pop_crossover_count = ALIGN_(
    state.population_size - pop_elite_count, xvr_growth);

  auto new_pop_capacity = pop_elite_count + pop_crossover_count;
  new_pop.reserve(new_pop_capacity);

  // pick elite
  for (size_t i = 0; i < pop_elite_count; ++i)
    new_pop.push_back(move(state.population[inds[i]]));

  // crossover, mutate the rest
  if (pop_crossover_count) {
    auto xvr_ofs = new_pop.size();
    new_pop.resize(new_pop_capacity);

    for_each(execution::par_unseq,
      xvr_iterator(new_pop, xvr_growth, xvr_ofs), xvr_iterator(new_pop),
      [
        &state,
        &inds,
        t = xvr_tournament(pop_elite_count, state.population_size)
      ]
      (auto& q) mutable {
        auto x1 = t(*state.prng, state.tournament_size);
        auto x2 = t(*state.prng, state.tournament_size);
        state.pxvr->exec(state.population[inds[x1]],
          state.population[inds[x2]], q, int(x1 - x2));
      });

    new_pop.resize(state.population_size);
    new_pop.shrink_to_fit();

    for_each(execution::par_unseq,
      next(new_pop.begin(), xvr_ofs), new_pop.end(),
      [&state](auto& g) mutable {
        state.pmtn->exec(g);
        g.set_id(state.uids.next_uid());
      });
  }

  state.population = move(new_pop);
  state.generation += 1;
}

template<typename T>
using unary_pred_fptr = bool (*)(T);
constexpr inline unary_pred_fptr<results_table::value_type> pred_std_isnan
  = std::isnan;

} // namespace

void app::REQUEST_HANDLER_MEM_DECL_(outcomes) {
  auto& s = state();
  auto in = request.data();

  if (in->data_size() && in->generation() != s.generation) {
    SPDLOG_LOGGER_WARN(_logger, "{} > unexpected generation {}!",
      in->client_name(), in->generation());
  }

  using clk_t = app_state::timeout_clock_t;
  auto now = clk_t::now();
  for (int i = 0, imax = in->data_size(); i < imax; ++i){
    auto& src = in->data(i);
    auto case_ind = s.cases_index.find(src.case_id());
    if (case_ind == s.cases_index.end()) {
      SPDLOG_LOGGER_WARN(_logger, "{} > unknown case id: {}, skipping.",
        in->client_name(), src.case_id());
      continue;
    }

    auto population_ind = s.population_index.find(src.genome_id());
    if (population_ind == s.population_index.end()) {
      SPDLOG_LOGGER_WARN(_logger, "{} > unknown genome id: {}, skipping.",
        in->client_name(), src.genome_id());
      continue;
    }

    s.timeouts[population_ind->second] = now;
    s.results[population_ind->second][case_ind->second] = src.rating();
  }

  [[maybe_unused]] future<void> state_write_sentry_;
  {
    size_t ready_count = 0;
    for (auto&& [i, row_from, row_to] : s.results) {
      if (none_of(row_from, row_to, pred_std_isnan)) {
        s.timeouts[i] = clk_t::time_point::max();
        ++ready_count;
      }
    }

    if (ready_count == s.population.size()) {
      generation_stats stats;
      next_generation(s, stats);
      state_write_sentry_ = async(launch::async,
        [&s] () mutable { persist_state(s); });

      SPDLOG_LOGGER_INFO(_logger, "Generation #{} is complete!\n"
          " Scores: {}; {}.",
        stats.generation, stats.score_best, stats.score_worst);

      on_generation_changed(s);
    }
  }

  auto out = google::protobuf::Arena::Create<pb::population>(in->GetArena());
  {
    out->set_generation(s.generation);

    DEBUG_(
      size_t count_resent{};
    )

    auto out_size = in->capacity();
    auto out_it = pb::inserter(out->mutable_data());
    for (auto n = s.population_size; n > 0 && out_size > 0; --n) {
      using namespace std::chrono_literals;
      if (now - s.timeouts[s.index] >= results_timeout) {

        DEBUG_(
          if (s.timeouts[s.index].time_since_epoch().count())
            ++count_resent;
        )
        
        s.timeouts[s.index] = now;
        *out_it++ = s.population[s.index];
        --out_size;
      }
      s.index = (s.index + 1) % s.population.size();
    }

    DEBUG_(
      if (count_resent) _logger->debug(
        "{} individuals resent due to timeout.", count_resent);
    )
  }
  response.append(out);
}

} // namespace marslander::trainer
