#include "state.h"

#include "base64.h"

#include <sstream>

namespace marslander {

#define  PTR_(x) reinterpret_cast<      char*>(x)
#define CPTR_(x) reinterpret_cast<const char*>(x)

void state::from_base64(const std::string& base64) {
  std::stringstream is(std::ios_base::in | std::ios_base::binary);
  {
    std::string dst(base64::decode(base64.data(), base64.size(), nullptr, 0),
      '\0');
    auto dst_ptr = reinterpret_cast<unsigned char*>(dst.data());
    base64::decode(base64.data(), base64.size(), dst_ptr, dst.size());
    is.str(dst);
  }

  // game_init_input
  size_t n;
  is >> n; surface.resize(n);
  is.read(PTR_(&surface[0]), n*sizeof(surface_type::value_type))
    .read(PTR_(&safe_area),  sizeof(safe_area_type));

  // game_turn_input
  is.read(PTR_(&fuel),     sizeof(scalar_type))
    .read(PTR_(&thrust),   sizeof(scalar_type))
    .read(PTR_(&tilt),     sizeof(scalar_type))
    .read(PTR_(&position), sizeof(position_type))
    .read(PTR_(&velocity), sizeof(velocity_type));

  // state
  is.read(PTR_(&safe_area_x),   sizeof(safe_area_x))
    .read(PTR_(&safe_area_alt), sizeof(safe_area_alt));
}

std::string state::to_base64() const {
  std::string src;
  {
    std::stringstream os(std::ios_base::out | std::ios_base::binary);

    // game_init_input
    auto n = surface.size();
    os << n;
    os.write(CPTR_(&surface[0]), n*sizeof(surface_type::value_type))
      .write(CPTR_(&safe_area),  sizeof(safe_area_type));

    // game_turn_input
    os.write(CPTR_(&fuel),     sizeof(scalar_type))
      .write(CPTR_(&thrust),   sizeof(scalar_type))
      .write(CPTR_(&tilt),     sizeof(scalar_type))
      .write(CPTR_(&position), sizeof(position_type))
      .write(CPTR_(&velocity), sizeof(velocity_type));

    // state
    os.write(CPTR_(&safe_area_x),   sizeof(safe_area_x))
      .write(CPTR_(&safe_area_alt), sizeof(safe_area_alt));

    src = os.str();
  }

  auto src_ptr = reinterpret_cast<unsigned char*>(src.data());
  std::string result(base64::encode(src_ptr, src.size(), nullptr, 0), '\0');
  base64::encode(src_ptr, src.size(), result.data(), result.size());
  return result;
}

} // namespace marslander
