#include <iostream>
#include <fstream>
#include <stddef.h>
#include "fhe.pb.h"
#include "google/protobuf/text_format.h"

using namespace std;
int main(int argc ,char**argv){
    Root proto;
    bool err;
    proto.set_test(atoi(argv[2]));
    proto.set_lt(atoi(argv[3]));
    std::string filename = argv[1]; 
    std::ofstream OutFile(filename); 
    // std::string buffer; 
    proto.SerializePartialToOstream(&OutFile);
    // std::cout<<buffer<<std::endl;
    proto.PrintDebugString();
    // err = google::protobuf::TextFormat::PrintToString(proto, &buffer);
    // if( ! err ){
    //     cout << "[+]Error: outputs to a string";
    // }
    
    // OutFile << buffer;  
    OutFile.close();  
}
