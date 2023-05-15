#include "postprocess/postprocess.h"
#include "proto/proto_setting.h"
#include "mutation_test/include/util.h"
#include <dirent.h>
using namespace google::protobuf;

#define SEED_NUM 4
#define MUTATION_TIMES 4000
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
    // create new seed and every seed has been mutated "MUTATION_TIMES" times
    if(argv[1][0] == 'c'){
        Root msg, add_msg;
        for(int i = 0;i < SEED_NUM;i++){
            int remain_size = MAX_BINARY_INPUT_SIZE;
            print_words({"-------------------", "original message", ToStr(i), "-------------------"}, 3, NO_STAR_LINE);
            msg.Clear();
            createRandomMessage(&msg, remain_size);
            remain_size = MAX_BINARY_INPUT_SIZE;
            add_msg.Clear();
            createRandomMessage(&add_msg, remain_size);
            int cnt = MUTATION_TIMES;
            while(cnt--){
                uint8_t *out_buf = nullptr;
                string data1 = msg.SerializeAsString(), data2 = add_msg.SerializeAsString();
                remain_size = MAX_BINARY_INPUT_SIZE;
                int new_size = afl_custom_fuzz(mutatorHelper, (uint8_t*)data1.c_str(), data1.size(),
                                    &out_buf, (uint8_t*)data2.c_str(), data2.size(), remain_size);
                // msg.PrintDebugString();
                uint8_t *post_out = nullptr;
                LoadProtoInput(USE_BINARY_PROTO, out_buf, new_size, &msg);
                string data = msg.SerializeAsString();
                new_size = data.size();
                for(int i = 0;i < data.size();i++)
                    temp[i] = data[i];
                temp[data.size()] = '\0';
                new_size = afl_custom_post_process(mutatorHelper, (unsigned char*)temp, new_size, &post_out);
                LoadProtoInput(USE_BINARY_PROTO, post_out, new_size, &msg);

            }
            ofstream out(text_seed_path + ToStr(i) + ".txt");
            string textData = msg.DebugString();
            out<<textData;
            out.close();
        }
        print_words({"create randomEngine seed:", ToStr(seed)},2);
    }
    // binary format to text format
    else if(argv[1][0] == 'r'){
        Root msg;
        string data = read_file_from_path(argv[2]);
        if(!LoadProtoInput(true, (const uint8_t *)data.c_str(), data.size(), &msg))
            ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
        string path = string(argv[2]);
        print_words({path}, 1);
        ofstream out(path + "_text");
        string textData = msg.DebugString();
        out << textData;
        out.close();
    }
    // text format to binary format
    else if(argv[1][0] == '1'){
        Root msg;
        string folder_path = text_seed_path;
        DIR* dirp = opendir(folder_path.c_str());
        struct dirent * dp;
        while ((dp = readdir(dirp)) != NULL) {
            if(dp->d_name[0] == '.') continue;
            std::string file_path = folder_path + "/" + dp->d_name;
            string data = read_file_from_path(file_path);
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

            ofstream out(bin_seed_path + (string)dp->d_name);
            // data = msg.DebugString();
            // out<<data;
            msg.SerializeToOstream(&out);
            out.close();
        }
        closedir(dirp);
    }
    return 0;
}
