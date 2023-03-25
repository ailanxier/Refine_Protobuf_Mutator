#ifndef SRC_UTF8_FIX_H_
#define SRC_UTF8_FIX_H_

#include <string>

#include "random.h"

namespace protobuf_mutator {
    void FixUtf8String(std::string* str, RandomEngine* random);
}  // namespace protobuf_mutator

#endif  // SRC_UTF8_FIX_H_
