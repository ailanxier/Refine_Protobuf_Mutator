#include "fhe_protobuf_mutator.h"

namespace fhe_protobuf_mutator {
    namespace {
        TestMessageHandler* GetMessageHandler(){
            static TestMessageHandler mhandler;
            return &mhandler;
        }
    }

    TestMessageHandler::TestMessageHandler(){
        temp = new char[100];
        of.open("proto_bout.txt", std::ios::trunc);
    }

    TestMessageHandler::~TestMessageHandler(){
        delete temp;
        of.close();
    }

    size_t TestMessageHandler::TransferMessageType(const TestRootMsg& input, unsigned char **out_buf){
        // of << input.param().num32() << " " << input.param().num64() << std::endl;
        std::string buffer;
        input.SerializePartialToString(&buffer);
        strcpy(temp, buffer.c_str());
        *out_buf = (unsigned char *)temp;
        return buffer.size();
    }

    // transfer Protobuf input to some interesting DATA and output the DATA to *out_buf
    DEFINE_AFL_PROTO_FUZZER(const TestRootMsg& input, unsigned char **out_buf, int index){
      /**
        * @param[in]  input    Protobuf Buffer containing the test case
        * @param[out] out_buf  Pointer to the buffer containing the test case after tranferance. 
        * @return              Size of the output buffer after processing or the needed amount.
        */
        return GetMessageHandler()->TransferMessageType(input, out_buf); 
    }
} // namespace test_fuzzer

