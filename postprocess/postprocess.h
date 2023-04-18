#ifndef FHE_PROTOBUF_MUTATOR_H_
#define FHE_PROTOBUF_MUTATOR_H_

#include <fstream>
#include <tuple>
#include "openfhe_ckks_postprocess.h"

// #define INITIAL_SIZE (500)
#define MAX(x, y) ( ((x) > (y)) ? (x) : y )
#define USE_BINARY_PROTO true

// Embedding buf_ in class MutateHelper here to prevent memory fragmentation caused by frequent memory allocation.
class AFLCustomHepler {
public: 
    AFLCustomHepler(int s){
        buf_  = static_cast<uint8_t*>(calloc(1, MAX_BINARY_INPUT_SIZE));
        if(!buf_) perror("MutateHelper init");
        temp = new char[MAX_BINARY_INPUT_SIZE];
    }
    ~AFLCustomHepler(){
        free(buf_);
        delete temp;
    }
    uint8_t* GetOutBuf() { return buf_; }
    char *temp;
    
private:
    uint8_t *buf_;  // for out_buf in afl_custom_fuzz() 
};

//Implementation of Crossover and Mutation of test cases in AFL_CustomProtoMutator.
int MutationOrCrossoverOnProtobuf(AFLCustomHepler *m, bool binary, unsigned char *buf, int buf_size, 
                            unsigned char **out_buf, unsigned char *add_buf, 
                            int add_buf_size, int max_size, Message* input1, Message* input2);

extern "C"{
    AFLCustomHepler *afl_custom_init(void *afl, unsigned int s){                                              
        AFLCustomHepler *mutate_helper = new AFLCustomHepler(s);                                                 
        std::ofstream seed_of("seed.txt", std::ios::trunc); 
        if(USE_SEED)                                                                      
            s = USE_SEED;     
        seed_of << s << std::endl;                                                                         
        seed_of.close();             
        getRandEngine()->Seed(s);                                                       
        return mutate_helper;                                                                              
    } 

    // Deinitialize everything                                                               
    void afl_custom_deinit(AFLCustomHepler *m){ delete m; }
    
    int afl_custom_fuzz(AFLCustomHepler *m, unsigned char *buf, int buf_size, unsigned char **out_buf,
                    unsigned char *add_buf, int add_buf_size, int max_size) {                 
        Root input1;                                                                                        
        Root input2;                                                                                    
        return MutationOrCrossoverOnProtobuf(m, USE_BINARY_PROTO, buf, buf_size, out_buf,                                 
                                    add_buf, add_buf_size, MAX_BINARY_INPUT_SIZE, &input1, &input2);                      
    }

    // A post-processing function to use right before AFL++ writes the test case to disk in order to execute the target.
    int afl_custom_post_process(AFLCustomHepler *m, unsigned char *buf, int buf_size, unsigned char**out_buf) {                                                                             
        Root input;                                                                              
        if (LoadProtoInput(USE_BINARY_PROTO, buf, buf_size, &input))                                                         
            return PostProcessMessage(input, out_buf, m->temp);                                                      
        return 0;                                                                                                       
    }
}                                                                 
#endif