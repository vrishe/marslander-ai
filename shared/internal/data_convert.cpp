#include "data_convert.h"

#include <algorithm>

namespace marslander::data {

using namespace std;

pair<uid_t, state> convert(const pb::landing_case& in) {
  state::surface_type s(in.surface_size());
  transform(in.surface().begin(), in.surface().end(),
    s.begin(), [](auto& p) -> state::surface_type::value_type { 
      return { p.x(), p.y() }; });
  using index_t = decltype(state::safe_area)::value_type;
  return make_pair<uid_t, state>(
    in.id(),
    {
      // init input
      move(s),
      {
        static_cast<index_t>(in.safe_area().start()),
        static_cast<index_t>(in.safe_area().end()),
      },
      // turn input
      in.fuel(),
      in.thrust(),
      in.tilt(),
      {
        in.position().x(),
        in.position().y(),
      },
      {
        in.velocity().x(),
        in.velocity().y(),
      },
      // state
      {
        in.surface().at(in.safe_area().start()).x(),
        in.surface().at(in.safe_area().end()).x(),
      },
      in.surface().at(in.safe_area().end()).y(),
      // output
      {}
    });
}

} // namespace marslander::data
