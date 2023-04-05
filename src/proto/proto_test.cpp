#include <iostream>
#include <fstream>
#include <stddef.h>
#include "openfhe_ckks.pb.h"
#include "afl_mutator.h"
#include "google/protobuf/text_format.h"

using namespace std;
using namespace google::protobuf;
using namespace protobuf_mutator;
#define COUT_RED "\033[31m"
#define COUT_GREEN "\033[32m"
#define COUT_END_COLOR "\033[0m"
inline void print_words(const vector<string> words, int highlight_pos = 0){
    int i = 1;
    for(const auto& word : words){
        if(i == highlight_pos)
            cout << COUT_GREEN << word << COUT_END_COLOR << " ";
        else 
            cout << word << " ";
        i++;
    }
    cout << endl;
}

int main(int argc ,char**argv){
    RootMsg msg;
    Mutator test;
    random_device rd;
    test.Seed(rd());
    int maxsize = 1000;
    createRandomMessage(&msg, maxsize, test.random());
    print_words({"maxsize:", to_string(maxsize), "size:", to_string(msg.ByteSizeLong())}, 4);
        // print_words({"==============", to_string(i), "=============="}, 2);
        // ofstream OutFile(to_string(i)+".txt", ios::out); 
        // msg.SerializePartialToOstream(&OutFile);
        msg.PrintDebugString();
        msg.Clear();
    // }
    
    return 0;
}
