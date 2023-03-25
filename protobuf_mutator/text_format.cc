#include "text_format.h"

#include "protobuf.h"

namespace protobuf_mutator {

  using protobuf::Message;
  using protobuf::TextFormat;

  bool ParseTextMessage(const uint8_t* data, size_t size, Message* output) {
    return ParseTextMessage({reinterpret_cast<const char*>(data), size}, output);
  }

  bool ParseTextMessage(const std::string& data, protobuf::Message* output) {
    output->Clear();
    TextFormat::Parser parser;
    parser.SetRecursionLimit(100);
    parser.AllowPartialMessage(true);
    parser.AllowUnknownField(true);
    if (!parser.ParseFromString(data, output)) {
      output->Clear();
      return false;
    }
    return true;
  }

  size_t SaveMessageAsText(const Message& message, uint8_t* data,
                          size_t max_size) {
    std::string result = SaveMessageAsText(message);
    if (result.size() <= max_size) {
      memcpy(data, result.data(), result.size());
      return result.size();
    }
    return 0;
  }

  std::string SaveMessageAsText(const protobuf::Message& message) {
    String tmp;
    if (!protobuf::TextFormat::PrintToString(message, &tmp)) return {};
    return tmp;
  }

}  // namespace protobuf_mutator
