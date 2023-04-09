#ifndef OPENFHE_CKKS_POSTPROCESS_H_
#define OPENFHE_CKKS_POSTPROCESS_H_
#include "protobuf_mutator/mutator.h"
#include "proto/proto_setting.h"
using namespace std;

int PostProcessMessage(const Root& msg, unsigned char **out_buf, char *temp){
    static int index = 1;
    string buffer;
    ofstream of("proto_bout.txt", std::ios::trunc);
    of << "================"<< index <<"================"<< endl;
    index++;
    buffer = msg.apisequence().DebugString();
    // auto api = msg.apisequence();
    of << buffer;
    // of<<"api byte_size: "<<buffer.size()<<endl;
    buffer = msg.SerializeAsString();
    // strcpy doesn't work, because it will stop at every '\0' in buffer
    for(int i = 0;i < buffer.size();i++)
        temp[i] = buffer[i];
    temp[buffer.size()] = '\0';
    *out_buf = (unsigned char *)temp;
    of.close();
    return buffer.size();
}

#endif