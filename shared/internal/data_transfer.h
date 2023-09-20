#ifndef SHARED_SHARED_H_
#include "internal/protobuf_streams.h"
#include "internal/traits_is_pointer.h"
#endif

#include "proto/message_info.h"

#include <google/protobuf/arena.h>
#include <google/protobuf/message_lite.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "sockpp/stream_socket.h"

#include <cassert>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace marslander::io {

using pb_msg_t = google::protobuf::MessageLite;
using const_pb_msg_t = const pb_msg_t;

struct any_message final {
  pb::message_id_t id;
  const_pb_msg_t* ptr;

  any_message() = default;

  any_message(const any_message& ) = default;
  any_message(      any_message&&) = default;

  any_message& operator=(const any_message& ) = default;
  any_message& operator=(      any_message&&) = default;

  template<typename T>
  any_message(const T* msg)
    : id(pb::message_info<T>::message_id),
      ptr(static_cast<const_pb_msg_t*>(msg))
    {}

  template<typename T,
  std::enable_if_t<
    !traits::is_pointer_v<T>,
    bool
  > = true>
  any_message(const T& msg) : any_message(&msg) {}

  template<typename T>
  any_message(const T* msg, const pb::packet_id_mapping& m)
    : id(m.at(msg->GetDescriptor()->full_name())),
      ptr(static_cast<const_pb_msg_t*>(msg))
    {}

  template<typename T>
  any_message(const T& msg, const pb::packet_id_mapping& m)
    : any_message(&msg, m) {}

  template<typename T>
  const T* as() const {
    assert(id == pb::message_info<T>::message_id);
    return static_cast<const T*>(ptr);
  }
};

class messages_bag final {

  std::unique_ptr<google::protobuf::Arena> _arena;
  std::vector<io::any_message> _msgs;

public:

  messages_bag(
    std::unique_ptr<google::protobuf::Arena>&& arena,
    std::vector<io::any_message>&& msgs)
    : _arena(std::move(arena)),
      _msgs(std::move(msgs))
    {}
  messages_bag(messages_bag&& other) = default;
  messages_bag() = default;

  messages_bag& operator=(messages_bag&& other) = default;

  bool empty() const noexcept { return _msgs.empty(); }
  size_t size() const noexcept { return _msgs.size(); }

  operator bool() const noexcept { return !empty(); }

  using iterator = decltype(_msgs)::const_iterator;
  auto begin() const { return _msgs.begin(); }
  auto   end() const { return _msgs.end(); }

};

class transfer_error final : public std::runtime_error
{
  int _errc;
  static std::string msg(const sockpp::socket& sock) {
    std::stringstream out;
    out << sock.last_error_str() << " (" << sock.last_error() << ").";
    return out.str();
  }
public:
  transfer_error(const sockpp::socket& sock)
    : std::runtime_error(msg(sock)), _errc{sock.last_error()} {}
  explicit transfer_error(const std::string& what)
    : std::runtime_error(what), _errc{} {}
  explicit transfer_error(const char* what)
    : std::runtime_error(what), _errc{} {}
  transfer_error(const transfer_error&) noexcept = default;
  transfer_error& operator=(const transfer_error&) noexcept = default;
  transfer_error(transfer_error&&) noexcept = default;
  transfer_error& operator=(transfer_error&&) noexcept = default;
  virtual ~transfer_error() noexcept = default;

  int error_code() const noexcept { return _errc; }
};

namespace detail_ {

bool write_any(google::protobuf::io::CodedOutputStream* dst,
  const any_message& m);

struct packet_header final {
  uint32_t messages_count;
};

struct identity_ final {
  template<typename V>
  V&& operator()(V&& v) { return std::forward<V>(v); }
};

template<typename It, typename Cvt = identity_>
auto& write_n(sockpp::stream_socket& dst, It from, uint32_t n,
    Cvt conv = Cvt()) {
  pb::StreamSocketOutputStream ssos(&dst);
  google::protobuf::io::CodedOutputStream cos(&ssos);

  detail_::packet_header hdr{n};
  cos.WriteRaw(static_cast<void*>(&hdr), sizeof(hdr));
  while (n-- > 0) {
    if (!detail_::write_any(&cos, conv(*from++)))
      throw transfer_error(dst);
  }

  return dst;
}

} // namespace detail_

sockpp::stream_socket& read(sockpp::stream_socket& src,
  const pb::messages_factory_mapping& m, google::protobuf::Arena* arena,
  std::vector<any_message>& msgs_out);

template<typename... Ts,
std::enable_if_t<
  std::conjunction_v<std::is_base_of<pb_msg_t, Ts>...>,
  bool
> = true>
auto& write(sockpp::stream_socket& dst, const Ts&... msgs) {
  auto l = { any_message(msgs)... };
  return detail_::write_n(dst, std::begin(l), l.size());
}

template<typename It,
std::enable_if_t<
  std::is_same_v<any_message,
    std::remove_cv_t<typename std::iterator_traits<It>::value_type>>,
  bool
> = true>
auto& write(sockpp::stream_socket& dst, It from, It to) {
  return detail_::write_n(dst, from, std::distance(from, to));
}

template<typename It,
std::enable_if_t<
  std::is_base_of_v<google::protobuf::Message,
    std::remove_pointer_t<typename std::iterator_traits<It>::value_type>>,
  bool
> = true>
auto& write(sockpp::stream_socket& dst, const pb::packet_id_mapping& m,
    It from, It to) {
  return detail_::write_n(dst, from, std::distance(from, to),
    [&m](auto& v) { return any_message(v, m); });
}

} // namespace marslander
