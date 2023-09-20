#include "global_includes.h"

#include "common.h"
#include "nn.h"

#include "nlohmann/json.hpp"

#include <fstream>

namespace marslander::data {

struct genome final {

  using genes_type = std::vector<fnum>;

  uid_t id = no_id;

  genes_type genes;

};

struct landing_case final {

  using value_type    = inum;
  using position_type = ipoint;
  using velocity_type = fpoint;
  using surface_type  = ipoints;

  uid_t id = no_id;

  value_type fuel;
  value_type thrust;
  value_type tilt;
  span<surface_type::size_type> safe_area;

  position_type position;
  velocity_type velocity;

  surface_type surface;

};

void convert(const genome& src, pb::genome& dst);
void convert(const landing_case& src, pb::landing_case& dst);

void convert_back(const pb::genome& src, genome& dst);
void convert_back(const pb::landing_case& src, landing_case& dst);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(genome,
  id, genes);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(landing_case,
  id, fuel, thrust, tilt, safe_area, position, velocity, surface);

} // namespace marslander::data
