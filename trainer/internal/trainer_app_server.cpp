#include "trainer_app.h"

#include <algorithm>

namespace marslander::trainer {

using namespace std;

void app::on_server_initialized() {
  using namespace std::placeholders;
#define MEM_FUN_HANDLER_(msg) server::handler<pb::msg>(\
    bind(&app::on_ ## msg ## _request, this, _1, _2))
  server::start(_args.port,
  {
    MEM_FUN_HANDLER_(cases),
    MEM_FUN_HANDLER_(outcomes),
  });

  cout << "Listening on port " << _args.port << endl;
}

void app::REQUEST_HANDLER_MEM_DECL_(cases) {
  auto& s = state();
  auto in = request.data();
  auto out = in->New(in->GetArena());
  auto out_data = out->mutable_data();
  out_data->Reserve(s.cases.size());
  copy(s.cases.begin(), s.cases.end(), pb::inserter(out_data));
  response.append(out);
}

} // namespace marslander::trainer
