#include "include/util.h"
#include "afl_mutator.h"
#include "proto/openfhe_ckks.pb.h"
using namespace std;
using namespace google::protobuf;
using namespace protobuf_mutator;
// 打印变异后的数据
DEFINE_AFL_PROTO_FUZZER(const RootMsg& input, unsigned char **out_buf, int index){
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
/**
 * @brief Compute the diff between two strings and print the diff like Git diff.
 *
 * @param text1 The first string to compare.
 * @param text2 The second string to compare.
 * @return A vector of string representing the differences between the two input strings.
 */
inline void print_diff(const string& text1, const string& text2){
    vector<string> result;
    // Split each string into individual lines
    vector<string> lines1;
    vector<string> lines2;
    size_t pos = 0;
    while (pos < text1.size()) {
        size_t end = text1.find_first_of("\n", pos);
        if (end == string::npos)
            end = text1.size();
        lines1.push_back(text1.substr(pos, end - pos));
        pos = end + 1;
    }
    pos = 0;
    while (pos < text2.size()) {
      size_t end = text2.find_first_of("\n", pos);
      if (end == string::npos)
          end = text2.size();
      lines2.push_back(text2.substr(pos, end - pos));
      pos = end + 1;
    }

    // Find the longest common subsequence of lines
    vector<vector<int>> lcs(lines1.size() + 1, vector<int>(lines2.size() + 1));
    for (size_t i = 0; i < lines1.size(); i++) 
      for (size_t j = 0; j < lines2.size(); j++) 
        if (lines1[i] == lines2[j])
            lcs[i + 1][j + 1] = lcs[i][j] + 1;
        else 
            lcs[i + 1][j + 1] = max(lcs[i][j + 1], lcs[i + 1][j]);

    // Build a list of diff operations (additions and deletions)
    size_t i = lines1.size();
    size_t j = lines2.size();
    while (i > 0 || j > 0) {
		if (i > 0 && j > 0 && lines1[i - 1] == lines2[j - 1]) {
			i--;
			j--;
		} else if (j > 0 && (i == 0 || lcs[i][j - 1] >= lcs[i - 1][j])) {
			result.push_back(COUT_GREEN + "+ " + lines2[j - 1] + COUT_END_COLOR);
			j--;
		} else {
			result.push_back(COUT_RED + "- " + lines1[i - 1] + COUT_END_COLOR);
			i--;
		}
    }

    // Reverse the order of the diff operations to match Git diff output
    reverse(result.begin(), result.end());
    for (const auto& line : result)
        print(line, 1, NO_STAR_LINE);
}

int main(int argc, char *argv[]){
    RootMsg msg;
    Mutator test;
    random_device rd;
    uint seed = rd();
    test.Seed(seed);
    size_t maxsize = 1000;
    createRandomMessage(&msg, maxsize);
    msg.PrintDebugString();
    string old = msg.DebugString();
    print_words({"maxsize:", to_string(maxsize), "size:", to_string(msg.ByteSizeLong())}, 4);
    maxsize = 1000;
    test.Mutate(&msg, maxsize);
    msg.PrintDebugString();
    string res = msg.DebugString();
    print_words({"maxsize:", to_string(maxsize), "msgsize:", to_string(msg.ByteSizeLong())}, 4);
    print_diff(old, res);
    print_words({"seed:", ToStr(seed)},2);
    msg.Clear();
    // RootMsg msg1, msg2;
    // auto read_file_from_path = [&](const string& path) {
    //     ostringstream buf; 
    //     ifstream input (path.c_str()); 
    //     buf << input.rdbuf(); 
    //     return buf.str();
    // };
    // 参数1和参数2是两个protobuf格式的二进制输入文件路径
    // string data1 = read_file_from_path(argv[1]);
    // if(!protobuf_mutator::LoadProtoInput(true, (const uint8_t *)data1.c_str(), data1.size(), &msg1))
    //   ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
    // string data2 = read_file_from_path(argv[2]);
    // if(!protobuf_mutator::LoadProtoInput(true, (const uint8_t *)data2.c_str(), data2.size(), &msg2)) 
    //   ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
    
    // // 设置种子
    // unsigned int seed = argc > 3 ? atoi(argv[3]) : 0;
    // string test_str = msg1.DebugString();
    // MutateHelper* mutatorHelper = afl_custom_init(nullptr, seed);
    // for(int i = 0; i < 50; i++) {
	// 	uint8_t *out_buf = nullptr;
	// 	size_t new_size = afl_custom_fuzz(mutatorHelper, (uint8_t*)data1.c_str(), data1.size(),
	// 						&out_buf, (uint8_t*)data2.c_str(), data2.size(), data1.size() + data2.size() + 600);
	// 	uint8_t *post_out = nullptr;
	// 	mutatorHelper->SetIndex(i);
	// 	// size_t post_out_size = afl_custom_post_process(mutatorHelper, out_buf, new_size, &post_out);
	// 	print_words({"==============", ToStr(i), "=============="}, 2, NO_STAR_LINE);
	// 	string mutate_str = string((char*)out_buf, new_size);
	// 	msg2.ParseFromString(mutate_str);
	// 	mutate_str = msg2.DebugString();
	// 	print_diff(test_str, mutate_str);
    // }
    // afl_custom_deinit(mutatorHelper);
    return 0;
}