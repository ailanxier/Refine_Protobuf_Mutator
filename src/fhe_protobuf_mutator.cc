#include "fhe_protobuf_mutator.h"
#include <fstream>
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

    size_t TestMessageHandler::TransferMessageType(const Root& input, unsigned char **out_buf){
        of << input.test() << " " << input.lt() << std::endl;
        std::string buffer;
        input.SerializePartialToString(&buffer);
        strcpy(temp, buffer.c_str());
        *out_buf = (unsigned char *)temp;
        return buffer.size();
        // if(input.test() == 123 || input.test() == 0 || input.test() == 1 || input.test() == 1073741824){
        //     Root now = input;
        //     now.set_test(107374182);
            
        //     std::string buffer;
        //     now.SerializeToString(&buffer);
        //     strcpy(temp, buffer.c_str());
        //     *out_buf = (unsigned char *)temp;
        //     // google::protobuf::TextFormat::PrintToString(now, &buffer);
        //     // of << buffer <<std::endl;
        //     return buffer.size();
        // }else{
        //     Root now = input;
        //     now.set_test(4444);
        //     std::string buffer;
        //     now.SerializeToString(&buffer);
        //     strcpy(temp, buffer.c_str());
        //     *out_buf = (unsigned char *)temp;
        //     return buffer.size();
        // }
    }

    // transfer Protobuf input to some interesting DATA and output the DATA to *out_buf
    DEFINE_AFL_PROTO_FUZZER(const Root& input, unsigned char **out_buf){
      /**
        * @param[in]  input    Protobuf Buffer containing the test case
        * @param[out] out_buf  Pointer to the buffer containing the test case after tranferance. 
        * @return              Size of the output buffer after processing or the needed amount.
        */
        return GetMessageHandler()->TransferMessageType(input, out_buf); 
    }
} // namespace test_fuzzer

