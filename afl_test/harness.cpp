#include <string>
#include <stdlib.h>
#include "../src/proto/fhe.pb.h"

using namespace std;
int main(int argc, char *argv[]) {
  string str;
  Root input;
  cin>>str;
  input.ParseFromString(str);
  int f = 0;
  for(int i = 0;i < 100;i++){
    if(i == input.lt()){
      f = 1;
      break;
    }
  }
  if(f) str[0] = '1';
  else str[0] = '0';
  if(input.test() == 4444){
    abort();
  }
  return 0;
}