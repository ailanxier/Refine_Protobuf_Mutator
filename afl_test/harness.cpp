#include <string>
#include <stdlib.h>
#include <fstream>
#include "../proto/proto_setting.h"

using namespace std;
int main(int argc, char *argv[]) {
    string str;
    OpenFHE_RootMsg input;
    ifstream in(argv[1]);
    input.ParseFromIstream(&in);
    int f = 0;
    for(int i = 0;i < 100;i++){
        if(i == input.param().multiplicativedepth()){
        f = 1;
        break;
        }
    }
    if(f) str[0] = '1';
    else str[0] = '0';
    if(input.param().digitsize() == 4444){
        abort();
    }
    return 0;
}