#include "postprocess/postprocess.h"
#include "proto/proto_setting.h"
#include "mutation_test/include/util.h"

using namespace std;
using namespace google::protobuf;
using namespace protobuf_mutator;

#define SEED_NUM 3
int main(int argc, char *argv[]){
    Root msg;
    Mutator test;
    random_device rd;
    uint seed = rd();
    test.Seed(seed);
    for(int i = 0;i < SEED_NUM;i++){
        int remain_size = MAX_BINARY_INPUT_SIZE;
        print_words({"-------------------", "original message", ToStr(i), "-------------------"}, 3, NO_STAR_LINE);
        msg.Clear();
        createRandomMessage(&msg, remain_size);
        msg.PrintDebugString();
        ofstream out(seed_path + ToStr(i) + ".txt");
        msg.SerializeToOstream(&out);
        out.close();
    }
    print_words({"create randomEngine seed:", ToStr(seed)},2);
    return 0;
}
