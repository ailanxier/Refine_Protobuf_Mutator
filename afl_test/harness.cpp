#include <string>
#include <stdlib.h>
#include <fstream>
#include "../src/proto/fhe.pb.h"

using namespace std;
int main(int argc, char *argv[]) {
  string str;
  TestRootMsg input;
  ifstream in(argv[1]);
  input.ParsePartialFromIstream(&in);
  // cout<<input.param().num32()<<endl;
  int f = 0;
  for(int i = 0;i < 100;i++){
    if(i == input.param().num32()){
      f = 1;
      break;
    }
  }
  if(f) str[0] = '1';
  else str[0] = '0';
  if(input.param().num64() == 4444){
    abort();
  }
  return 0;
}