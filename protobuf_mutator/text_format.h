#ifndef SRC_TEXT_FORMAT_H_
#define SRC_TEXT_FORMAT_H_

#include <string>

#include "protobuf.h"

namespace protobuf_mutator {
    // Text serialization of protos.
    bool ParseTextMessage(const uint8_t* data, size_t size, protobuf::Message* output);
    bool ParseTextMessage(const std::string& data, protobuf::Message* output);
    size_t SaveMessageAsText(const protobuf::Message& message, uint8_t* data, size_t max_size);
    std::string SaveMessageAsText(const protobuf::Message& message);
}  // namespace protobuf_mutator

#endif  // SRC_TEXT_FORMAT_H_
