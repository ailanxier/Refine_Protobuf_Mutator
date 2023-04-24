// #include "postprocess/postprocess.h"
// #include "proto/proto_setting.h"
// #include <dirent.h>
// #include <chrono>
// using namespace std;
// using namespace google::protobuf;
// using namespace protobuf_mutator;
// std::chrono::_V2::system_clock::time_point start;
// #define MUTATION_TIMES 100
// char temp[MAX_BUFFER_SIZE];
// inline int64_t getTime(){ 
//     auto end = std::chrono::high_resolution_clock::now();
//     auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
//     start = end;
//     return duration.count();
// }

// int main(int argc, char *argv[]){
//     auto read_file_from_path = [&](const string& path) {
//         ostringstream buf; 
//         ifstream input (path.c_str()); 
//         buf << input.rdbuf(); 
//         return buf.str(); 
//     };
//     TESTRoot msg1, msg2, temp_msg;
//     random_device rd;
//     uint seed = rd();
//     AFLCustomHepler* mutatorHelper = afl_custom_init(nullptr, seed);
//     int remain_size = MAX_BINARY_INPUT_SIZE;

//     DIR* dirp = opendir(mutation_path.c_str());
//     struct dirent * dp;
//     int64_t total_time = 0;
//     while ((dp = readdir(dirp)) != NULL) {
//         if(dp->d_name[0] == '.') continue;
//         string file1_path = mutation_path + "/" + dp->d_name;
//         string file2_path = crossover_path + "/" + dp->d_name;
//         string data1 = read_file_from_path(file1_path);
//         string data2 = read_file_from_path(file2_path);
//         msg1.Clear();
//         msg2.Clear();
//         LoadProtoInput(true, (const uint8_t *)data1.c_str(), data1.size(), &msg1);
//         LoadProtoInput(true, (const uint8_t *)data1.c_str(), data1.size(), &msg2);
//         data1 = msg1.SerializeAsString(), data2 = msg2.SerializeAsString();
//         start = std::chrono::high_resolution_clock::now();
//         int64_t once_total_time = 0;
//         for(int i = 0;i < MUTATION_TIMES;i++){
//             uint8_t *out_buf = nullptr;
//             remain_size = MAX_BINARY_INPUT_SIZE;
//             int new_size = afl_custom_fuzz(mutatorHelper, (uint8_t*)data1.c_str(), data1.size(),
//                                 &out_buf, (uint8_t*)data2.c_str(), data2.size(), remain_size);
//             auto t = getTime();
//             once_total_time += t;
//             // cout<<"Time: "<<t<<endl;
//         }
//         cout<<"Average Time: "<<once_total_time / MUTATION_TIMES<<endl;
//         total_time += once_total_time;
//     }
//     cout<<"========= Total Average Time: "<<total_time / MUTATION_TIMES<< " ========="<<endl;
//     closedir(dirp);
//     return 0;
// }
