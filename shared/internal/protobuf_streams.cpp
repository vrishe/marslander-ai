#include "protobuf_streams.h"

#include "sockpp/stream_socket.h"

namespace marslander::pb {

// ===================================================================

StreamSocketInputStream::StreamSocketInputStream(sockpp::stream_socket* input, int block_size)
    : copying_input_(input), impl_(&copying_input_, block_size) {}

bool StreamSocketInputStream::Next(const void** data, int* size) {
  return impl_.Next(data, size);
}

void StreamSocketInputStream::BackUp(int count) { impl_.BackUp(count); }

bool StreamSocketInputStream::Skip(int count) { return impl_.Skip(count); }

int64_t StreamSocketInputStream::ByteCount() const { return impl_.ByteCount(); }

StreamSocketInputStream::CopyingStreamSocketInputStream::CopyingStreamSocketInputStream(
    sockpp::stream_socket* input)
    : input_(input) {}

StreamSocketInputStream::CopyingStreamSocketInputStream::~CopyingStreamSocketInputStream() {}

int StreamSocketInputStream::CopyingStreamSocketInputStream::Read(void* buffer,
                                                        int size) {
  return input_->read(buffer, size);
}

// ===================================================================

StreamSocketOutputStream::StreamSocketOutputStream(sockpp::stream_socket* output, int block_size)
    : copying_output_(output), impl_(&copying_output_, block_size) {}

StreamSocketOutputStream::~StreamSocketOutputStream() { impl_.Flush(); }

bool StreamSocketOutputStream::Next(void** data, int* size) {
  return impl_.Next(data, size);
}

void StreamSocketOutputStream::BackUp(int count) { impl_.BackUp(count); }

int64_t StreamSocketOutputStream::ByteCount() const { return impl_.ByteCount(); }

StreamSocketOutputStream::CopyingStreamSocketOutputStream::CopyingStreamSocketOutputStream(
    sockpp::stream_socket* output)
    : output_(output) {}

StreamSocketOutputStream::CopyingStreamSocketOutputStream::~CopyingStreamSocketOutputStream() {
}

bool StreamSocketOutputStream::CopyingStreamSocketOutputStream::Write(const void* buffer,
                                                            int size) {
  return output_->write_n(buffer, size) == size;
}

// ===================================================================

} // namespace marslander::pb
