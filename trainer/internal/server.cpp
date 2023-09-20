#include "server.h"
#include "global_includes.h"

#include "sockpp/tcp_acceptor.h"

#include <google/protobuf/arena.h>

#include <algorithm>
#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include <vector>

namespace marslander::trainer {

namespace server {

using namespace sockpp;
using namespace std;

namespace {

static logger_ptr logger_;
static inline logger_ptr logger() {
  if (!logger_) logger_ = spdlog::get(loggers::server_logger);
  return logger_;
}

class response_sink_impl;
struct response_wait_state final {

  using counter_t = atomic_size_t;
  using data_element_t = unique_ptr<vector<io::any_message>>;

  friend class response_sink_impl;

  response_wait_state(counter_t::value_type size)
    : _data(size), _next_sink_index(0), _count(0),
        _ready(_ready_promise.get_future()) {}

  data_element_t::element_type wait() {
    if (_ready.valid()) _ready.wait();

    data_element_t::element_type result;
    for (auto& item : _data) {
      if (item) {
        copy(item->begin(), item->end(), back_inserter(result));
      }
    }
    return result;
  }

  response_sink_impl get_sink();

private:

  void notify_one()
  {
    if (++_count == _data.size())
      _ready_promise.set_value();
  }

  vector<data_element_t> _data;
  size_t _next_sink_index;

  counter_t _count;
  promise<void> _ready_promise;
  future<void> _ready;

};

class response_sink_impl final : public response_sink {

  using data_element_t = response_wait_state::data_element_t;

  shared_ptr<response_wait_state> _ws_ptr;
  data_element_t* _msgs_ptr;

public:

  response_sink_impl(response_wait_state* ws_ptr, data_element_t& msgs)
    : _ws_ptr(ws_ptr, [](auto* v){v->notify_one();}),
      _msgs_ptr(&msgs)
    {}

  response_sink_impl(const response_sink_impl&  other) = default;
  response_sink_impl(      response_sink_impl&& other) = default;

  ~response_sink_impl() override = default;

  response_sink& append(io::any_message&& msg) override {
    auto& msgs = *_msgs_ptr;
    if (!msgs) msgs = make_unique<data_element_t::element_type>();
    msgs->push_back(move(msg));
    return *this;
  }

};

response_sink_impl response_wait_state::get_sink() {
  return { this, _data.at(_next_sink_index++) };
}

const pb::messages_factory_mapping factory_
  = pb::build_messages_factory_mapping();

atomic_int threads_count;
inline int max_threads_count() {
  return max<int>(thread::hardware_concurrency() - 1, 1);
}

void server_thread(tcp_acceptor acc, looper& l,
    const handlers_map_t& handlers) {
  logger()->info("New server thread #{} begins! ({})",
    this_thread::get_id(), ++threads_count);
  [[maybe_unused]] scope::on_exit threads_count_sentry_([]{
    logger()->info("Server thread #{} finished. ({})",
      this_thread::get_id(), --threads_count);
  });

  inet_address peer;
  bool spawn;

loop: {
    tcp_socket sock = acc.accept(&peer);

    if (spawn = threads_count < max_threads_count()) {
      std::thread(&server_thread, move(acc), std::ref(l), handlers).detach();
    }

    try {

      SPDLOG_LOGGER_INFO(logger(), "{} > Incoming connection", peer);
      std::vector<io::any_message> messages;
      auto arena = std::make_unique<google::protobuf::Arena>();
      io::read(sock, factory_, arena.get(), messages);
      auto msgs = make_shared<io::messages_bag>(
        std::move(arena), std::move(messages));
      if (msgs->size() <= 0) {
        logger()->warn("{} : Empty request.", peer);
        return;
      }
      // TODO: throw on msgs->size() <= 0?
      SPDLOG_LOGGER_INFO(logger(),
        "{} > Received request ({} messages).", peer, msgs->size());

      response_wait_state ws(msgs->size());

      ITERZ_((*msgs), i);
      while(!ITERZ_END(i)) {
        auto& msg = *i_it++;
        l.post(bind(handlers.at(msg.id),
          request_resources {msgs, &msg}, move(ws.get_sink())));
      }

      auto data = ws.wait();
      // TODO: throw on data.empty()?
      io::write(sock, data.begin(), data.end());
      SPDLOG_LOGGER_INFO(logger(),
        "{} < Written response ({} messages).", peer, data.size());

    }
    catch(const exception& ex) {
      logger()->error("{} : {}", peer, ex.what());
    }
  }

  if (!spawn) {
    SPDLOG_LOGGER_INFO(logger(),
      "Server thread #{} restart.", this_thread::get_id());
    goto loop;
  }
}

} // namespace

void start(in_port_t port, handlers_map_t&& handlers) {
  tcp_acceptor acc(port);
  if (!acc) throw io::transfer_error(acc);

  std::thread(&server_thread, move(acc), std::ref(looper::current()),
    move(handlers)).detach();
}

} // namespace server

} // namespace marslander::trainer
