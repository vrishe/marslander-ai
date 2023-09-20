#pragma once

#ifndef SHARED_PROTO_PROTOTYPE_ID_H_
#define SHARED_PROTO_PROTOTYPE_ID_H_

#include "proto/transmitted.pb.h"

#include <google/protobuf/arena.h>
#include <google/protobuf/message_lite.h>

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <type_traits>

namespace marslander::pb {

typedef uint32_t message_id_t;

inline constexpr message_id_t no_message_id = 0;

#define MESSAGE_INFO_STRUCT_BODY_(message_id_val, full_name_val)\
  message_info() = delete;\
  static constexpr message_id_t message_id = (message_id_val);\
  static std::string full_name() { return (full_name_val); }

template<typename T>
struct message_info {
MESSAGE_INFO_STRUCT_BODY_(no_message_id, "<unknown>")
};

#define MESSAGE_LITE_INFO_(type, message_id, full_name)\
template<>\
struct message_info<marslander::pb::type> {\
MESSAGE_INFO_STRUCT_BODY_(message_id, full_name)\
}

#define MESSAGE_INFO_(type, message_id)\
MESSAGE_LITE_INFO_(type, message_id, type::GetDescriptor()->full_name())

MESSAGE_INFO_(cases, 1);
MESSAGE_INFO_(outcomes, 2);
MESSAGE_INFO_(population, 3);
// TODO: more types go here.

typedef std::map<std::string, message_id_t> packet_id_mapping;
#define std_string_2_message_id_t_(type)\
{ message_info<type>::full_name(), message_info<type>::message_id }
inline packet_id_mapping build_packet_id_mapping() {
  return {
    std_string_2_message_id_t_(cases),
    std_string_2_message_id_t_(outcomes),
    std_string_2_message_id_t_(population),
  };
}

typedef std::function<
  google::protobuf::MessageLite*(google::protobuf::Arena*)
> factory_func;
typedef std::map<message_id_t, factory_func> messages_factory_mapping;
#define message_id_t_2_factory_func_(type)\
{ message_info<type>::message_id, [](google::protobuf::Arena* arena)\
  -> google::protobuf::MessageLite* {\
    return google::protobuf::Arena::CreateMessage<type>(arena); } }
inline messages_factory_mapping build_messages_factory_mapping() {
  return {
    message_id_t_2_factory_func_(cases),
    message_id_t_2_factory_func_(outcomes),
    message_id_t_2_factory_func_(population),
  };
}

} // namespace marslander::pb

#endif // SHARED_PROTO_PROTOTYPE_ID_H_
