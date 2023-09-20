#include "global_includes.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <random>

namespace marslander::trainer {

using score_t = decltype(std::declval<pb::outcomes_stats>().rating());
using gene_t  = decltype(std::declval<pb::genome>().genes(0));

static inline fnum genome_sway = 1e4;

namespace detail_ {

template<typename T>
inline T inc(T value) {
 return std::nextafter(value, std::numeric_limits<T>::max());
}

inline std::uniform_real_distribution<double> uniform01 {.0, inc(1.)};
inline std::uniform_real_distribution<double> uniform01_strict {inc(.0), 1.};

} // namespace detail_

template<typename Rng>
void crossover_heuristic(Rng&& rng, const pb::genome& parent_a,
    const pb::genome& parent_b, pb::genome& child,
    double ratio) {

  using namespace std;

  const auto genes_count = parent_a.genes_size();
  assert(genes_count == parent_b.genes_size());

  // parent A MUST have a better fitness
  auto a_genes = parent_a.genes(),
       b_genes = parent_b.genes();

  using namespace std::placeholders;

  auto child_genes = child.mutable_genes();
  child_genes->Clear();
  child_genes->Reserve(genes_count);
  transform(a_genes.begin(), a_genes.end(), b_genes.begin(),
    pb::inserter(child_genes), bind(lerp<gene_t>, _2, _1, gene_t{ratio}));
}

template<typename Rng>
void crossover_intermediate(Rng&& rng, const pb::genome& parent_a,
    const pb::genome& parent_b, pb::genome& child,
    double ratio) {

  using namespace std;

  const auto genes_count = parent_a.genes_size();
  assert(genes_count == parent_b.genes_size());

  auto a_genes = parent_a.genes(),
       b_genes = parent_b.genes();

  using namespace std::placeholders;

  auto child_genes = child.mutable_genes();
  child_genes->Clear();
  child_genes->Reserve(genes_count);
  const auto t = gene_t{ratio * detail_::uniform01(rng)};
  transform(a_genes.begin(), a_genes.end(), b_genes.begin(),
    pb::inserter(child_genes), bind(lerp<gene_t>, _1, _2, t));
}

// Laplace crossover as described in section 2.1 of
// 'A real coded genetic algorithm for solving integer and mixed integer
// optimization problems' - Deep et al.
template<typename Rng>
void crossover_Laplace(Rng&& rng,
    const pb::genome& parent_x1, const pb::genome& parent_x2,
    pb::genome& child_y1, pb::genome& child_y2,
    double a, double b) {
  using namespace std;

  const auto genes_count = parent_x1.genes_size();
  assert(genes_count == parent_x2.genes_size());

  auto x1_genes = parent_x1.genes(),
       x2_genes = parent_x2.genes();

  vector<gene_t> beta;
  beta.reserve(genes_count);
  transform(x1_genes.begin(), x1_genes.end(), x2_genes.begin(),
    back_inserter(beta), [a, b, &rng](auto x1, auto x2) {
      auto r = detail_::uniform01(rng);
      auto d = abs(double(x1) - double(x2));
      return gene_t{d * ( a + double((r > .5) - (r <= .5))
        * b * log(detail_::uniform01_strict(rng)) )};
    });

  auto
  child_genes = child_y1.mutable_genes();
  child_genes->Clear();
  child_genes->Reserve(genes_count);
  transform(x1_genes.begin(), x1_genes.end(), beta.begin(),
    pb::inserter(child_genes), std::plus<>());

  child_genes = child_y2.mutable_genes();
  child_genes->Clear();
  child_genes->Reserve(genes_count);
  transform(x2_genes.begin(), x2_genes.end(), beta.begin(),
    pb::inserter(child_genes), std::plus<>());
}

template<typename Rng>
void crossover_scattered(Rng&& rng, const pb::genome& parent_a,
    const pb::genome& parent_b, pb::genome& child,
    double t) {

  const auto genes_count = parent_a.genes_size();
  assert(genes_count == parent_b.genes_size());

  auto a_genes = parent_a.genes(),
       b_genes = parent_b.genes();

  auto child_genes = child.mutable_genes();
  child_genes->Clear();
  child_genes->Reserve(genes_count);
  std::transform(a_genes.begin(), a_genes.end(), b_genes.begin(),
    pb::inserter(child_genes), [t, &rng](auto p1, auto p2) {
      return detail_::uniform01(rng) <= t ? p1 : p2;
    });
}

template<typename Rng>
void mutation_Gaussian(Rng&& rng, pb::genome& child,
    double t, double mean, double stddev) {
  std::normal_distribution<> d_norm{mean, stddev};
  auto child_genes = child.mutable_genes();
  for (auto& u : *child_genes) {
    auto x = d_norm(rng);
    if (std::abs(x) >= mean + t) u = gene_t{u + x};
  }
}

// Power mutation as described in section 2.2 of
// 'A real coded genetic algorithm for solving integer and mixed integer
// optimization problems' - Deep et al.
template<typename Rng>
void mutation_power(Rng&& rng, pb::genome& child,
    double p, double xl, double xu) {
  const auto s = std::pow(detail_::uniform01(rng), p);

  auto child_genes = child.mutable_genes();
  for (auto& u : *child_genes) {
    auto v = double(u), vxl = v-xl, xuv = xu-v, t = vxl/xuv,
      r = detail_::uniform01(rng);

    // no explicit NaN-check for t is necessary
    u = gene_t { v + s * ( (t >= r) * xuv - (t < r) * vxl ) };
  }
}

template<typename Rng>
void mutation_uniform(Rng&& rng, pb::genome& child,
    double rate, double a, double b) {
  auto child_genes = child.mutable_genes();
  const auto s = b - a;
  for (auto& u : *child_genes) {
    if (detail_::uniform01(rng) <= rate)
      u = a + s * detail_::uniform01(rng);
  }
}

template<typename T>
struct child_output_query {
  virtual T& next_child() = 0;
  virtual ~child_output_query() = default;
};

struct algo_meta {
  const size_t growth;
};

template<typename T>
struct crossover_algo_tmpl_ {
  using arg_t = const T;
  using output_query_t = child_output_query<T>;
  virtual void exec(arg_t&, arg_t&, output_query_t&, int) = 0;
  virtual ~crossover_algo_tmpl_() = default;
  const algo_meta& meta() const { return _meta; }
protected:
  crossover_algo_tmpl_(const algo_meta& meta) : _meta{meta} {}
private:
  const algo_meta _meta;
};

using crossover_algo = crossover_algo_tmpl_<pb::genome>;

namespace algo::xvr {

#define DECL_COMPARE_PARENTS_(x1, x2, cmp) \
  auto [px1, px2] {(cmp) <= 0 ? std::tie(x1, x2) : std::tie(x2, x1)}

template<typename Rng>
class heuristic final : public crossover_algo {
  using base_ = crossover_algo;
  std::shared_ptr<Rng> _prng;
  const double _ratio;
public:
  heuristic(std::shared_ptr<Rng> prng, double ratio)
    : crossover_algo({1}), _prng(std::move(prng)), _ratio(ratio) { }
  void exec(base_::arg_t& x1, base_::arg_t& x2,
      base_::output_query_t& q, int cmp) override {
    DECL_COMPARE_PARENTS_(x1, x2, cmp);
    crossover_heuristic(*_prng, px1, px2, q.next_child(), _ratio);
  }
};

template<typename Rng>
class intermediate final : public crossover_algo {
  using base_ = crossover_algo;
  std::shared_ptr<Rng> _prng;
  const double _ratio;
public:
  intermediate(std::shared_ptr<Rng> prng, double ratio)
    : crossover_algo({1}), _prng(std::move(prng)), _ratio(ratio) { }
  void exec(base_::arg_t& x1, base_::arg_t& x2,
      base_::output_query_t& q, int) override {
    crossover_intermediate(*_prng, x1, x2, q.next_child(), _ratio);
  }
};

template<typename Rng>
class laplace final : public crossover_algo {
  using base_ = crossover_algo;
  std::shared_ptr<Rng> _prng;
  const double _a, _b;
public:
  laplace(std::shared_ptr<Rng> prng, double a, double b)
    : crossover_algo({2}), _prng(std::move(prng)), _a{a}, _b{b} { }
  void exec(base_::arg_t& x1, base_::arg_t& x2,
      base_::output_query_t& q, int) override {
    auto& primary_child = q.next_child();
    crossover_Laplace(*_prng, x1, x2,
      primary_child, q.next_child(), _a, _b);
  }
};

template<typename Rng>
class scattered final : public crossover_algo {
  using base_ = crossover_algo;
  std::shared_ptr<Rng> _prng;
  const double _t;
public:
  scattered(std::shared_ptr<Rng> prng, double t)
    : crossover_algo({1}), _prng(std::move(prng)), _t{t} { }
  void exec(base_::arg_t& x1, base_::arg_t& x2,
      base_::output_query_t& q, int cmp) override {
    DECL_COMPARE_PARENTS_(x1, x2, cmp);
    crossover_scattered(*_prng, px1, px2, q.next_child(), _t);
  }
};

} // namespace algo::crossover

template<typename T>
struct mutation_algo_tmpl_ {
  using arg_t = T;
  virtual void exec(arg_t&) = 0;
  virtual ~mutation_algo_tmpl_() = default;
};

using mutation_algo = mutation_algo_tmpl_<pb::genome>;

namespace algo::mtn {

struct none final : mutation_algo {
  using base_ = mutation_algo;
  void exec(base_::arg_t& g) override {}
};

template<typename Rng>
class gaussian final : public mutation_algo {
  using base_ = mutation_algo;
  std::shared_ptr<Rng> _prng;
  const double _t, _mean, _stddev;
public:
  gaussian(std::shared_ptr<Rng> prng,
      double t, double mean, double stddev)
    : _prng(std::move(prng)), _t(t*stddev), _mean(mean), _stddev(stddev) { }
  void exec(base_::arg_t& g) override {
    mutation_Gaussian(*_prng, g, _t, _mean, _stddev);
  }
};

template<typename Rng>
class power final : public mutation_algo {
  using base_ = mutation_algo;
  std::shared_ptr<Rng> _prng;
  const double _p, _xl, _xu;
public:
  power(std::shared_ptr<Rng> prng, double p, double xl, double xu)
    : _prng(std::move(prng)), _p{p}, _xl{xl}, _xu{xu} { }
  void exec(base_::arg_t& g) override {
    mutation_power(*_prng, g, _p, _xl, _xu);
  }
};

template<typename Rng>
class uniform final : public mutation_algo {
  using base_ = mutation_algo;
  std::shared_ptr<Rng> _prng;
  const double _a, _b, _rate;
public:
  uniform(std::shared_ptr<Rng> prng, double a, double b, double rate)
    : _prng(std::move(prng)), _a{a}, _b{b}, _rate{rate} { }
  void exec(base_::arg_t& g) override {
    mutation_uniform(*_prng, g, _rate, _a, _b);
  }
};

} // namespace algo::mtn

} // namespace marslander::trainer
