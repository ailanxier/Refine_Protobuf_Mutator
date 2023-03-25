#ifndef PROTO_UTIL_H_
#define PROTO_UTIL_H_

#include <stddef.h>
#include <cstdint>
#include <functional>
#include <type_traits>
#include "protobuf.h"

namespace protobuf_mutator {

  size_t CustomProtoMutator(bool binary, uint8_t* data, size_t size, size_t max_size, unsigned int seed,
                            protobuf::Message* input);
  size_t CustomProtoCrossOver(bool binary, const uint8_t* data1, size_t size1,
                              const uint8_t* data2, size_t size2, uint8_t* out,
                              size_t max_out_size, unsigned int seed,
                              protobuf::Message* input1,
                              protobuf::Message* input2);

  // data -> input
  bool LoadProtoInput(bool binary, const uint8_t* data, size_t size, protobuf::Message* input);

  void RegisterPostProcessor(const protobuf::Descriptor* desc,
      std::function<void(protobuf::Message* message, unsigned int seed)> callback);

  template <class Proto>
  struct PostProcessorRegistration {
    PostProcessorRegistration(const std::function<void(Proto* message, unsigned int seed)>& callback) {
      RegisterPostProcessor(Proto::descriptor(), 
          [callback](protobuf::Message* message, unsigned int seed) {
            callback(static_cast<Proto*>(message), seed);
          });
    }
  };

}  // namespace protobuf_mutator

#endif  // SRC_LIBFUZZER_LIBFUZZER_MACRO_H_
