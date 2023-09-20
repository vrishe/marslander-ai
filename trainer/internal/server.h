#include "global_includes.h"

#include "sockpp/platform.h"

#include <functional>
#include <map>
#include <memory>

namespace marslander::trainer {

namespace server {

struct response_sink {

  virtual response_sink& append(io::any_message&&) = 0;

protected:

  virtual ~response_sink() = default;

};

struct request_resources {

  std::shared_ptr<io::messages_bag> msgs;
  const io::any_message* msg_ptr;

};

template<typename T>
struct request final {

  request(const request_resources& res) : _res(res) {}

  request(const request&  other) = default;
  request(      request&& other) = default;

  pb::message_id_t id() const { return _res.msg_ptr->id; }
  const T* data() const { return _res.msg_ptr->as<T>(); }

private:

  const request_resources& _res;

};

using handlers_map_t = std::map<
  pb::message_id_t,
  std::function<void(const request_resources&, response_sink&)>
>;

template<typename M, typename H>
handlers_map_t::value_type handler(H&& handler) {
  return std::make_pair(pb::message_info<M>::message_id,
    [h = std::forward<H>(handler)](auto& res, auto& sink) {
      h(request<M>(res), sink);
    });
}

void start(in_port_t port, handlers_map_t&& handlers);

} // namespace server

} // namespace marslander::trainer
