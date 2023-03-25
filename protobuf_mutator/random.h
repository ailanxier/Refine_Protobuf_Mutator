#ifndef SRC_RANDOM_H_
#define SRC_RANDOM_H_

#include <random>

namespace protobuf_mutator {
    using RandomEngine = std::minstd_rand;
}  // namespace protobuf_mutator

#endif  // SRC_RANDOM_H_
