#include "postprocess/postprocess.h"
#include "proto/proto_setting.h"
#include <dirent.h>
using namespace google::protobuf;

#define SEED_NUM 1000
#define MUTATION_TIMES 15
char temp[MAX_BUFFER_SIZE];
int main(int argc, char *argv[]){
    auto read_file_from_path = [&](const string& path) {
        ostringstream buf; 
        ifstream input (path.c_str()); 
        buf << input.rdbuf(); 
        return buf.str(); 
    };
    random_device rd;
    uint seed = rd();
    AFLCustomHepler* mutatorHelper = afl_custom_init(nullptr, seed);
    TESTRoot mutation_msg, crossover_msg;
    for(int i = 0;i < SEED_NUM;i++){
        int remain_size = MAX_BINARY_INPUT_SIZE;
        cout << "------------------- original message " << i << " -------------------" << endl;
        mutation_msg.Clear();
        createRandomMessage(&mutation_msg, remain_size);
        remain_size = MAX_BINARY_INPUT_SIZE;
        crossover_msg.Clear();
        createRandomMessage(&crossover_msg, remain_size);
        int cnt = MUTATION_TIMES;
        // while(cnt--){
        //     uint8_t *out_buf = nullptr;
        //     string data1 = mutation_msg.SerializeAsString(), data2 = crossover_msg.SerializeAsString();
        //     remain_size = MAX_BINARY_INPUT_SIZE;
        //     int new_size = afl_custom_fuzz(mutatorHelper, (uint8_t*)data1.c_str(), data1.size(),
        //                         &out_buf, (uint8_t*)data2.c_str(), data2.size(), remain_size);
        //     LoadProtoInput(USE_BINARY_PROTO, out_buf, new_size, &mutation_msg);
        //     data1 = mutation_msg.SerializeAsString();
        //     remain_size = MAX_BINARY_INPUT_SIZE;
        //     new_size = afl_custom_fuzz(mutatorHelper, (uint8_t*)data2.c_str(), data2.size(),
        //                         &out_buf, (uint8_t*)data1.c_str(), data1.size(), remain_size);
        //     LoadProtoInput(USE_BINARY_PROTO, out_buf, new_size, &crossover_msg);
        // }
        ofstream out(mutation_path + to_string(i), ios::trunc | ios::binary);
        mutation_msg.SerializeToOstream(&out);
        out.close();
        // mutation_msg.PrintDebugString();
        ofstream add_out(crossover_path + to_string(i), ios::trunc | ios::binary);
        crossover_msg.SerializeToOstream(&out);
        out.close();
    }        
    return 0;
}
