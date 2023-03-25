#ifndef SRC_BINARY_FORMAT_H_
#define SRC_BINARY_FORMAT_H_

#include <string>

#include "protobuf.h"

namespace protobuf_mutator {

// Binary serialization of protos.
bool ParseBinaryMessage(const uint8_t* data, size_t size,
                        protobuf::Message* output);
bool ParseBinaryMessage(const std::string& data, protobuf::Message* output);
size_t SaveMessageAsBinary(const protobuf::Message& message, uint8_t* data,
                           size_t max_size);
std::string SaveMessageAsBinary(const protobuf::Message& message);

}  // namespace protobuf_mutator

#endif  // SRC_BINARY_FORMAT_H_
