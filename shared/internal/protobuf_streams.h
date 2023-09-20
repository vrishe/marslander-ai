#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "sockpp/stream_socket.h"

namespace marslander::pb {

struct CodedInputStreamLimitScope final {
  using stream_t = ::google::protobuf::io::CodedInputStream;
  CodedInputStreamLimitScope(stream_t* cis, size_t limit)
    : _cis(cis), _l{cis->PushLimit(limit)} {}
  ~CodedInputStreamLimitScope() { _cis->PopLimit(_l); }
private:
  const stream_t::Limit _l;
  stream_t* const _cis;
};

class StreamSocketInputStream final
  : public ::google::protobuf::io::ZeroCopyInputStream {
public:

  explicit StreamSocketInputStream(sockpp::stream_socket* input, int block_size = -1);
  StreamSocketInputStream(const StreamSocketInputStream&) = delete;
  StreamSocketInputStream& operator=(const StreamSocketInputStream&) = delete;

  // implements ZeroCopyInputStream ----------------------------------
  bool Next(const void** data, int* size) override;
  void BackUp(int count) override;
  bool Skip(int count) override;
  int64_t ByteCount() const override;

private:
  class CopyingStreamSocketInputStream final
      : public google::protobuf::io::CopyingInputStream {
   public:
    CopyingStreamSocketInputStream(sockpp::stream_socket* input);
    CopyingStreamSocketInputStream(const CopyingStreamSocketInputStream&) = delete;
    CopyingStreamSocketInputStream& operator=(const CopyingStreamSocketInputStream&) =
        delete;
    ~CopyingStreamSocketInputStream() override;

    // implements CopyingInputStream ---------------------------------
    int Read(void* buffer, int size) override;
    // (We use the default implementation of Skip().)

   private:
    // The socket.
    sockpp::stream_socket* input_;
  };

  CopyingStreamSocketInputStream copying_input_;
  google::protobuf::io::CopyingInputStreamAdaptor impl_;
};

// ===================================================================

class StreamSocketOutputStream final
  : public ::google::protobuf::io::ZeroCopyOutputStream {
public:

  explicit StreamSocketOutputStream(sockpp::stream_socket* stream, int block_size = -1);
  StreamSocketOutputStream(const StreamSocketOutputStream&) = delete;
  StreamSocketOutputStream& operator=(const StreamSocketOutputStream&) = delete;
  ~StreamSocketOutputStream() override;

  // implements ZeroCopyOutputStream ---------------------------------
  bool Next(void** data, int* size) override;
  void BackUp(int count) override;
  int64_t ByteCount() const override;

private:
  class CopyingStreamSocketOutputStream final
      : public google::protobuf::io::CopyingOutputStream {
   public:
    CopyingStreamSocketOutputStream(sockpp::stream_socket* output);
    CopyingStreamSocketOutputStream(const CopyingStreamSocketOutputStream&) = delete;
    CopyingStreamSocketOutputStream& operator=(const CopyingStreamSocketOutputStream&) =
        delete;
    ~CopyingStreamSocketOutputStream() override;

    // implements CopyingOutputStream --------------------------------
    bool Write(const void* buffer, int size) override;

   private:
    // The socket.
    sockpp::stream_socket* output_;
  };

  CopyingStreamSocketOutputStream copying_output_;
  google::protobuf::io::CopyingOutputStreamAdaptor impl_;
};

} // namespace marslander::pb
