#include "util.h"
#include "afl_mutator.h"
#include "proto/fhe.pb.h"
using namespace std;

// 打印变异后的数据
DEFINE_AFL_PROTO_FUZZER(const Root& input, unsigned char **out_buf, int index){
    /**
    * @param[in]  input    Protobuf Buffer containing the test case
    * @param[out] out_buf  Pointer to the buffer containing the test case after tranferance. 
    * @return              Size of the output buffer after processing or the needed amount.
    */
    string buffer = input.DebugString();
    print_words({"==============", ToStr(index), "=============="}, 2, NO_STAR_LINE);
    print(buffer, 1, NO_STAR_LINE);
    return buffer.size();
}

int main(int argc, char *argv[]){
    Root msg1, msg2;
    auto read_file_from_path = [&](const string& path) {
        ostringstream buf; 
        ifstream input (path.c_str()); 
        buf << input.rdbuf(); 
        return buf.str();
    };
    // 参数1和参数2是两个protobuf格式的二进制输入文件路径
    string data1 = read_file_from_path(argv[1]);
    if(!protobuf_mutator::LoadProtoInput(true, (const uint8_t *)data1.c_str(), data1.size(), &msg1))
      ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
    string data2 = read_file_from_path(argv[2]);
    if(!protobuf_mutator::LoadProtoInput(true, (const uint8_t *)data2.c_str(), data2.size(), &msg2)) 
      ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
    
    // 设置种子
    unsigned int seed = argc > 3 ? atoi(argv[3]) : 0;

    MutateHelper* mutatorHelper = afl_custom_init(nullptr, seed);
    for(int i = 0; i < 30; i++) {
      uint8_t *out_buf = nullptr;
      size_t new_size = afl_custom_fuzz(mutatorHelper, (uint8_t*)data1.c_str(), data1.size(),
                        &out_buf, (uint8_t*)data2.c_str(), data2.size(), data1.size() + data2.size() + 600);
      uint8_t *post_out = nullptr;
      mutatorHelper->SetIndex(i);
      size_t post_out_size = afl_custom_post_process(mutatorHelper, out_buf, new_size, &post_out);
    }
    afl_custom_deinit(mutatorHelper);
    return 0;
}