#include "data_transfer.h"

namespace marslander::io {

namespace detail_ {

struct message_header final {
  pb::message_id_t message_id;
  size_t message_size;
};

static inline bool read_any(google::protobuf::io::CodedInputStream* src,
    const pb::messages_factory_mapping& m, google::protobuf::Arena* arena,
    any_message& result) {
  message_header hdr{};
  src->ReadRaw(static_cast<void*>(&hdr), sizeof(hdr));
  result.id = hdr.message_id;
  auto msg = m.at(result.id)(arena);
  result.ptr = msg;
  [[maybe_unused]] pb::CodedInputStreamLimitScope sentry_(src, hdr.message_size);
  return msg->ParseFromCodedStream(src);
}

bool write_any(google::protobuf::io::CodedOutputStream* dst,
    const any_message& m) {
  message_header hdr{m.id, m.ptr->ByteSizeLong()};
  dst->WriteRaw(static_cast<void*>(&hdr), sizeof(hdr));
  return m.ptr->SerializeToCodedStream(dst);
}

} // namespace detail_

sockpp::stream_socket& read(sockpp::stream_socket& src,
    const pb::messages_factory_mapping& m, google::protobuf::Arena* arena,
    std::vector<any_message>& msgs_out) {
  pb::StreamSocketInputStream ssis(&src);
  google::protobuf::io::CodedInputStream cis(&ssis);

  detail_::packet_header hdr{};
  cis.ReadRaw(static_cast<void*>(&hdr), sizeof(hdr));

  constexpr size_t max_messages_count = 128;
  if (hdr.messages_count > max_messages_count) {
    std::stringstream out("Data packet header specifies", std::ios_base::ate);
    out << hdr.messages_count << " messages to read; "
      << max_messages_count << " is the allowed maximum.";
    throw transfer_error(out.str());
  }

  msgs_out.resize(hdr.messages_count);
  for (auto& item : msgs_out) {
    if (!detail_::read_any(&cis, m, arena, item))
      throw transfer_error(src);
  }

  return src;
}

} // namespace marslander
