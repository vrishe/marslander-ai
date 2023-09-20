#include "proto/genome.pb.h"
#include "constants.h"
#include "nn.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <random>
#include <type_traits>

namespace marslander::nn {

namespace detail_ {

using namespace std;

template<typename T, class Meta, template<typename> class Activator>
struct distribution_s_ {
  static inline auto get() {
    // Xavier
    auto l = sqrt(6)/sqrt(Meta::input_size + Meta::neurons_count);
    return uniform_real_distribution<T>{-l,l};
  }
};

template<typename T, class Meta>
struct distribution_s_<T, Meta, activators::ReLU> {
  static inline auto get() {
    // He et al.
    auto l = std::sqrt(2./Meta::input_size);
    return normal_distribution<T>{0,l};
  }
};

template<typename GenesIter>
inline void fill_n_zeroes(GenesIter* genes, size_t n) {
  while (n-- != 0) genes->Add(0);
}

template<typename Rng, typename D, typename GenesIter>
inline void fill_n_rand(Rng&& rng, D& d, GenesIter* genes, size_t n) {
  while (n-- != 0) genes->Add(d(rng));
}

template<template<typename> class Activator, class Layer, typename Rng, typename GenesIter>
inline void add_layer(Rng&& rng, GenesIter* genes) {
  using layer_meta_t = typename Layer::meta_type;
  auto h0 = distribution_s_<
      typename Layer::value_type,
      layer_meta_t,
      Activator
    >::get();

  fill_n_zeroes(genes, layer_meta_t::layer_weights_offset);
  fill_n_rand(rng, h0, genes, layer_meta_t::layer_size
    - layer_meta_t::layer_weights_offset);
}

} // namespace

template<template<typename> class Activator, typename Rng>
void randomize(Rng&& rng, pb::genome& genome) {
  using namespace detail_;
  using brain_t = DFF_base<fnum>;
  using brain_meta_t = typename brain_t::meta_type;

  auto genes = genome.mutable_genes();
  genes->Clear();
  genes->Reserve(brain_meta_t::total_size);
  add_layer<Activator, decltype(brain_t::hidden0)>(rng, genes);
  add_layer<Activator, decltype(brain_t::hidden1)>(rng, genes);
  add_layer<Activator, decltype(brain_t::output )>(rng, genes);
  genes->Truncate(genome.genes_size());
}

template<typename Rng>
void randomize_naive(Rng&& rng, pb::genome& genome, fnum swing = 1) {
  using namespace detail_;
  using brain_t = DFF_base<fnum>;
  using brain_meta_t = typename brain_t::meta_type;

  auto d = uniform_real_distribution<
    brain_t::value_type> {-swing,swing};

  auto genes = genome.mutable_genes();
  genes->Clear();
  genes->Reserve(brain_meta_t::total_size);
  fill_n_rand(rng, d, genes, brain_meta_t::hidden0_meta::layer_size);
  fill_n_rand(rng, d, genes, brain_meta_t::hidden1_meta::layer_size);
  fill_n_rand(rng, d, genes, brain_meta_t::output_meta ::layer_size);
  genes->Truncate(genome.genes_size());
}

} // namespace marslander::nn
