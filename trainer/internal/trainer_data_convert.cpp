#include "trainer_data.h"

#include <algorithm>

namespace marslander::data {

void convert(const genome& src, pb::genome& dst) {
  dst.set_id(src.id);

  dst.clear_genes();
  auto dst_genes = dst.mutable_genes();
  dst_genes->Reserve(src.genes.size());
  for (const auto& p_src : src.genes)
    dst_genes->Add(p_src);
}

void convert(const landing_case& src, pb::landing_case& dst) {
  dst.set_id(src.id);
  dst.set_fuel(src.fuel);
  dst.set_thrust(src.thrust);
  dst.set_tilt(src.tilt);
  auto safe_area = dst.mutable_safe_area();
  safe_area->set_start(src.safe_area.start);
  safe_area->set_end(src.safe_area.end);
  auto position = dst.mutable_position();
  position->set_x(src.position.x);
  position->set_y(src.position.y);
  auto velocity = dst.mutable_velocity();
  velocity->set_x(src.velocity.x);
  velocity->set_y(src.velocity.y);

  dst.clear_surface();
  auto dst_surface = dst.mutable_surface();
  dst_surface->Reserve(src.surface.size());
  for (const auto& p_src : src.surface) {
    auto p_dst = dst_surface->Add();
    p_dst->set_x(p_src.x);
    p_dst->set_y(p_src.y);
  }
}

void convert_back(const pb::genome& src, genome& dst) {
  dst.id = src.id();

  auto& dst_genes = dst.genes;
  dst_genes.resize(src.genes_size());
  auto& src_genes = src.genes();
  std::copy(src_genes.begin(), src_genes.end(), dst_genes.begin());
}

void convert_back(const pb::landing_case& src, landing_case& dst) {
  dst.id = src.id();
  dst.fuel = src.fuel();
  dst.thrust = src.thrust();
  dst.tilt = src.tilt();
  const auto& safe_area = src.safe_area();
  dst.safe_area = { 
    static_cast<landing_case::surface_type::size_type>(safe_area.start()),
    static_cast<landing_case::surface_type::size_type>(safe_area.end())
  };
  const auto& position = src.position();
  dst.position = { position.x(), position.y() };
  const auto& velocity = src.velocity();
  dst.velocity = { velocity.x(), velocity.y() };

  auto& dst_surface = dst.surface;
  dst_surface.resize(src.surface_size());
  auto& src_surface = src.surface();
  std::transform(src_surface.begin(), src_surface.end(), dst_surface.begin(),
    [](const auto& p_src) -> landing_case::surface_type::value_type { 
      return { p_src.x(), p_src.y() };
    });
}

} // namespace marslander::data
