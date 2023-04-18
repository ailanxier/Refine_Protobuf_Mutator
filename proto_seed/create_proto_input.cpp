#include "postprocess/postprocess.h"
#include "proto/proto_setting.h"
#include "mutation_test/include/util.h"

using namespace std;
using namespace google::protobuf;
using namespace protobuf_mutator;

#define SEED_NUM 10
char temp[MAX_BINARY_INPUT_SIZE + 2];
int main(int argc, char *argv[]){
    auto read_file_from_path = [&](const string& path) {
        ostringstream buf; 
        ifstream input (path.c_str()); 
        buf << input.rdbuf(); 
        return buf.str(); 
    };
    Root msg;
    random_device rd;
    uint seed = rd();
    AFLCustomHepler* mutatorHelper = afl_custom_init(nullptr, seed);
    if(argv[1][0] == 'c'){
        
        for(int i = 0;i < SEED_NUM;i++){
            int remain_size = MAX_BINARY_INPUT_SIZE;
            print_words({"-------------------", "original message", ToStr(i), "-------------------"}, 3, NO_STAR_LINE);
            msg.Clear();
            createRandomMessage(&msg, remain_size);
            // msg.PrintDebugString();
            uint8_t *post_out = nullptr;
            string data = msg.SerializeAsString();
            auto new_size = data.size();
            for(int i = 0;i < data.size();i++)
                temp[i] = data[i];
            temp[data.size()] = '\0';
            cout<<new_size<<endl;
            new_size = afl_custom_post_process(mutatorHelper, (unsigned char*)temp, new_size, &post_out);
            string str = string((char*)post_out, new_size);
            LoadProtoInput(USE_BINARY_PROTO, post_out, new_size, &msg);            
            ofstream out(text_seed_path + ToStr(i) + ".txt");
            string textData = msg.DebugString();
            out<<textData;
            out.close();
        }
        print_words({"create randomEngine seed:", ToStr(seed)},2);
    }else if(argv[1][0] == 'r'){
        string data = read_file_from_path(argv[2]);
        if(!LoadProtoInput(true, (const uint8_t *)data.c_str(), data.size(), &msg))
            ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
        string path = string(argv[2]);
        print_words({path}, 1);
        ofstream out(path + ".txt");
        string textData = msg.DebugString();
        out << textData;
        out.close();
    }else{
        for(int i = 0;i < SEED_NUM;i++){
            string data = read_file_from_path(text_seed_path + ToStr(i) + ".txt");
            if(!LoadProtoInput(false, (const uint8_t *)data.c_str(), data.size(), &msg))
                ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
            
            // uint8_t *post_out = nullptr;
            // data = msg.SerializeAsString();
            // auto new_size = data.size();
            // for(int i = 0;i < data.size();i++)
            //     temp[i] = data[i];
            // temp[data.size()] = '\0';
            // new_size = afl_custom_post_process(mutatorHelper, (unsigned char*)temp, new_size, &post_out);
            // string str = string((char*)post_out, new_size);
            // LoadProtoInput(USE_BINARY_PROTO, post_out, new_size, &msg);   
            // print_words({"-------------------", "original message", ToStr(i), "-------------------"}, 3, NO_STAR_LINE);
            // msg.PrintDebugString();

            ofstream out(bin_seed_path + ToStr(i) + ".txt");
            // data = msg.DebugString();
            // out<<data;
            msg.SerializeToOstream(&out);
            out.close();
        }
    }
    return 0;
}
