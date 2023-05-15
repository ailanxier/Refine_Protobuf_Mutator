#include "postprocess.h"

int MutationOrCrossoverOnProtobuf(AFLCustomHepler *m, bool binary, unsigned char *buf, int buf_size, 
                            unsigned char **out_buf, unsigned char *add_buf, int add_buf_size, 
                                       int max_size, Message* input1, Message* input2) {
    int now = GetRandomIndex(10), out_size;
    if(now <= 5){
        memcpy(m->GetOutBuf(), buf, buf_size);
        out_size = CustomProtoMutate(binary, m->GetOutBuf(), buf_size, max_size, input1);
        *out_buf = m->GetOutBuf();
    }else{
        // Crossover buf and add_buf and store the outbuf to m.buf_
        out_size = CustomProtoCrossOver(binary, buf, buf_size, add_buf, add_buf_size, m->GetOutBuf(), 
                                        max_size, input1, input2);
        *out_buf = m->GetOutBuf();
    }
    return out_size;
}
