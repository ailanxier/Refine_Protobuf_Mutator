#include "include/util.h"
#include "postprocess/postprocess.h"
#include "proto/proto_setting.h"
using namespace std;
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
    int pos = 0;
    while (pos < text1.size()) {
        int end = text1.find_first_of("\n", pos);
        if (end == string::npos)
            end = text1.size();
        lines1.push_back(text1.substr(pos, end - pos));
        pos = end + 1;
    }
    pos = 0;
    while (pos < text2.size()) {
      int end = text2.find_first_of("\n", pos);
      if (end == string::npos)
          end = text2.size();
      lines2.push_back(text2.substr(pos, end - pos));
      pos = end + 1;
    }

    // Find the longest common subsequence of lines
    vector<vector<int>> lcs(lines1.size() + 1, vector<int>(lines2.size() + 1));
    for (int i = 0; i < lines1.size(); i++) 
      for (int j = 0; j < lines2.size(); j++) 
        if (lines1[i] == lines2[j])
            lcs[i + 1][j + 1] = lcs[i][j] + 1;
        else 
            lcs[i + 1][j + 1] = max(lcs[i][j + 1], lcs[i + 1][j]);

    // Build a list of diff operations (additions and deletions)
    int i = lines1.size();
    int j = lines2.size();
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
    Root msg1, msg2;
    random_device rd;
    uint seed = rd();
    const int MAX = MAX_BINARY_INPUT_SIZE;
    int remain_size = MAX;
    print_words({"randomEngine seed:", ToStr(seed)},2);
    auto read_file_from_path = [&](const string& path) {
        ostringstream buf; 
        ifstream input (path.c_str()); 
        buf << input.rdbuf(); 
        return buf.str(); 
    };

    //参数1和参数2是两个protobuf格式的二进制输入文件路径
    string data1 = read_file_from_path(seed_path + "1.txt");
    if(!LoadProtoInput(USE_BINARY_PROTO, (const uint8_t *)data1.c_str(), data1.size(), &msg1))
      ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
    string data2 = read_file_from_path(seed_path + "2.txt");
    if(!LoadProtoInput(USE_BINARY_PROTO, (const uint8_t *)data2.c_str(), data2.size(), &msg2)) 
      ERR_EXIT("[afl_custom_post_process] LoadProtoInput Error\n");
    
    // 设置种子
    AFLCustomHepler* mutatorHelper = afl_custom_init(nullptr, seed);
    print_words({"-------------------", "original message1", "-------------------"}, 2, NO_STAR_LINE);
    // createRandomMessage(&msg1, remain_size);
    msg1.PrintDebugString();
    // remain_size = MAX;
    // print_words({"-------------------", "original message2", "-------------------"}, 2, NO_STAR_LINE);
    // // createRandomMessage(&msg2, remain_size);
    // msg2.PrintDebugString();
    
    // string data1 = msg1.SerializeAsString(), data2 = msg2.SerializeAsString();
    string test_str = msg1.DebugString();

    remain_size = MAX;
    for(int i = 0; i < 100; i++) {
		uint8_t *out_buf = nullptr; 
		int new_size = afl_custom_fuzz(mutatorHelper, (uint8_t*)data1.c_str(), data1.size(),
							&out_buf, (uint8_t*)data2.c_str(), data2.size(), remain_size);
		uint8_t *post_out = nullptr;
		// print_words({"=======================", ToStr(i), "====================="}, 2, NO_STAR_LINE);
        new_size = afl_custom_post_process(mutatorHelper, out_buf, new_size, &post_out);
        string str = string((char*)post_out, new_size);

		msg1.ParseFromString(str);
		// data1 = msg1.DebugString();
        // print_words({"-------------------", "msg1", "-------------------"}, 2);
        // msg1.PrintDebugString();
		// print_diff(test_str, data1);
        data1 = msg1.SerializeAsString();
    }
    print_words({"-------------------", "msg1", "-------------------"}, 2);
    msg1.PrintDebugString();
    afl_custom_deinit(mutatorHelper);
    return 0;
}