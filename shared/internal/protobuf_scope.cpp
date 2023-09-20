#include "protobuf_scope.h"

#include <google/protobuf/message_lite.h>

namespace marslander {

protobuf_scope::protobuf_scope() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;
}

protobuf_scope::~protobuf_scope() {
  google::protobuf::ShutdownProtobufLibrary();
}

} // namespace marslander
