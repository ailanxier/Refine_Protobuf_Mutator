#ifndef FHE_PROTOBUF_MUTATOR_H_
#define FHE_PROTOBUF_MUTATOR_H_

#include "proto/fhe.pb.h"
#include "afl_mutator.h"
#include <string>
#include <fstream>

namespace fhe_protobuf_mutator {
    class TestMessageHandler{
        public:
            TestMessageHandler();
            ~TestMessageHandler();
            size_t TransferMessageType(const Root& input, unsigned char **out_buf);
            std::ofstream of;
            char *temp;
    };
} //namespace test_fuzzer


#endif