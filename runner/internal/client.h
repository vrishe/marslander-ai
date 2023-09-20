#include "global_includes.h"

#include "sockpp/tcp_connector.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

namespace marslander::runner {

namespace client {

using response = io::messages_bag;

namespace detail_ {

logger_ptr logger();

response read_response(sockpp::stream_socket& sock);

constexpr inline auto delay(std::chrono::seconds(5));
template<typename Rep, typename Period>
void repeat(const std::string& reason,
    const std::chrono::duration<Rep, Period>& d) {
  SPDLOG_LOGGER_INFO(logger(), "{} Next attempt in {}.", reason, d);
  std::this_thread::sleep_for(d);
}

#define CLIENT_LOOP_REP_(cond, msg)\
  if ((cond)) { using namespace ::marslander::runner::client::detail_;\
    repeat(msg, delay); continue; }

} // namespace detail_

template<typename... Ts>
response request(
    const sockpp::inet_address& addr, const Ts&... msgs) {

  sockpp::tcp_connector conn(addr);
  if (!conn) throw io::transfer_error(conn);

  auto peer = conn.peer_address();
  io::write(conn, msgs...);
  SPDLOG_LOGGER_TRACE(logger(), "{} < Request sent", peer);
  auto conn_h = conn.release();
  {
    sockpp::socket sd_sock(conn_h);
    if (!sd_sock.shutdown(SHUT_WR))
      throw io::transfer_error(sd_sock);
  }
  sockpp::stream_socket rd_sock(conn_h);
  auto through = [peer](response&& r) {
    if (r.empty()) throw std::logic_error("Empty response unacceptable.");
    SPDLOG_LOGGER_TRACE(logger(), "{} > Response received ({} messages)", peer, r.size());
    return std::move(r);
  };
  return through(detail_::read_response(rd_sock));
}

template<typename T0, typename... Ts,
std::enable_if_t<
  std::conjunction_v<std::is_default_constructible<T0>,
    std::is_default_constructible<Ts>...>,
  bool
> = true>
response request(const sockpp::inet_address& addr) {
  return request(addr, T0(), Ts()...);
}

} // namespace client

} // namespace marslander::runner
