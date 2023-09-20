#ifndef SHARED_SHARED_H_
#include "internal/uid_source.h"
#endif

#include "marslander/state.h"
#include "proto/genome.pb.h"
#include "proto/landing_case.pb.h"

#include "common.h"
#include "nn.h"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

namespace marslander {

template<typename T>
void to_json(nlohmann::json& j, const point<T>& p) {
  j["x"] = p.x;
  j["y"] = p.y;
}

template<typename T>
void from_json(const nlohmann::json& j, point<T>& p) {
  j.at("x").get_to(p.x);
  j.at("y").get_to(p.y);
}

template<typename T>
void to_json(nlohmann::json& j, const span<T>& s) {
  j["start"] = s.start;
  j["end"] = s.end;
}

template<typename T>
void from_json(const nlohmann::json& j, span<T>& s) {
  j.at("start").get_to(s.start);
  j.at("end").get_to(s.end);
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(game_init_input,
  surface, safe_area);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(game_turn_input,
  fuel, thrust, tilt, position, velocity);

namespace data {

template<typename T>
struct convert_f {
  using DFF = nn::DFF<T>;

  std::pair<uid_t, DFF> operator()(const pb::genome& g) const {
    auto& genes = g.genes();
    assert(genes.size() == DFF::meta_type::total_size);

    auto result = std::make_pair(g.id(), DFF{});
    std::copy(genes.begin(), genes.end(),
      reinterpret_cast<typename DFF::value_type*>(&result.second));

    return result;
  }
};

std::pair<uid_t, state> convert(const pb::landing_case&);

} // namespace data

#define PRETTY_JSON std::setw(2) << std::setfill(' ')
#define JSON_SESSION_DUMP(check,generation,cases_count,population_size,\
elite_count,tournament_size,crossover,mutation,cases,population)\
nlohmann::json{\
  {"check",check},\
  {"generation",generation},\
  {"cases_count",cases_count},\
  {"population_size",population_size},\
  {"elite_count",elite_count},\
  {"tournament_size",tournament_size},\
  {"crossover",crossover},\
  {"mutation",mutation},\
  {"cases",json_cases},\
  {"population",json_population},\
}
#define JSON_REPLAY(case_id,gene_id,outcome,base64_state,surface,turns)\
nlohmann::json{\
  {"case_id",case_id},\
  {"gene_id",gene_id},\
  {"outcome",outcome},\
  {"state",base64_state},\
  {"surface",surface},\
  {"turns",turns},\
}

} // namespace marslander
