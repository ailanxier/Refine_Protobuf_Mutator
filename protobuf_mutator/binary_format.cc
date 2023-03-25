#include "binary_format.h"

namespace protobuf_mutator {

using protobuf::Message;

bool ParseBinaryMessage(const uint8_t* data, size_t size, Message* output) {
  return ParseBinaryMessage({reinterpret_cast<const char*>(data), size}, output);
}

bool ParseBinaryMessage(const std::string& data, protobuf::Message* output) {
  output->Clear();
  if (!output->ParsePartialFromString(data)) {
    output->Clear();
    return false;
  }
  return true;
}

size_t SaveMessageAsBinary(const Message& message, uint8_t* data, size_t max_size) {
  std::string result = SaveMessageAsBinary(message);
  if (result.size() <= max_size) {
    memcpy(data, result.data(), result.size());
    return result.size();
  }
  return 0;
}

std::string SaveMessageAsBinary(const protobuf::Message& message) {
  String tmp;
  if (!message.SerializePartialToString(&tmp)) return {};
  return tmp;
}

}  // namespace protobuf_mutator
