#include "client.h"
#include "global_includes.h"

#include "sockpp/stream_socket.h"

#include <google/protobuf/arena.h>

#include <memory>
#include <vector>

namespace marslander::runner {

namespace client {

using namespace sockpp;
using namespace std;

namespace {

const pb::messages_factory_mapping factory_
  = pb::build_messages_factory_mapping();

} // namespace

namespace detail_ {

static logger_ptr logger_;
logger_ptr logger() {
  if (!logger_) logger_ = spdlog::get(loggers::client_logger);
  return logger_;
}

response read_response(sockpp::stream_socket& sock) {
  std::vector<io::any_message> messages;
  auto arena = std::make_unique<google::protobuf::Arena>();
  io::read(sock, factory_, arena.get(), messages);
  return response(std::move(arena), std::move(messages));
}

} // namespace detail_

} // namespace client

} // namespace marslander::runner
