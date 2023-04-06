#include "afl_mutator.h"

#define INITIAL_SIZE (100)
#define MAX(x, y) ( ((x) > (y)) ? (x) : y )

namespace protobuf_mutator{
    namespace aflplusplus {
        namespace {
            int GetRandom(int s){
                static std::default_random_engine generator(s);
                std::uniform_int_distribution<int> uniform_dist(1, 10);
                return uniform_dist(generator);
            }
        } // namespace

        MutateHelper::MutateHelper(int s) {
            buf_  = static_cast<uint8_t*>(calloc(1, INITIAL_SIZE));
            if(!buf_) perror("MutateHelper init");
            seed_ = s;
        }

        uint8_t* MutateHelper::ReallocBuf(int length) {
            len_ = MAX(length, INITIAL_SIZE);
            buf_ = static_cast<uint8_t*>(realloc(buf_, len_));
            if(!buf_) perror("MutateHelper realloc");
            return buf_;
        }

        int AFL_CustomProtoMutator(MutateHelper *m, bool binary, unsigned char *buf, int buf_size, 
                                    unsigned char **out_buf, unsigned char *add_buf, 
                                    int add_buf_size, int max_size, Message* input1, Message* input2) {                   
            // (PC) probability of crossover : 0.8 (0.6~1.0)
            // (PM) probability of mutation  : 0.2 
            int now = GetRandomIndex(10);
            // std::ofstream random_of("random.txt", std::ios::out | std::ios::app);
            // random_of << now << std::endl;
            // random_of.close();
            if(now <= 5){
                memcpy(m->ReallocBuf(std::max(max_size, buf_size)), buf, buf_size);
                m->SetLen(CustomProtoMutate(binary, m->GetOutBuf(), buf_size, max_size, input1));
            }else{
                // Crossover buf and add_buf and store the outbuf to m.buf_
                m->ReallocBuf(std::max({max_size, buf_size, add_buf_size}));
                m->SetLen(CustomProtoCrossOver(binary, buf, buf_size, add_buf, add_buf_size, m->GetOutBuf(), 
                                                max_size, input1, input2));
            }
            *out_buf = m->GetOutBuf();
            return m->GetLen();
        }

    } // aflplusplus
} // protobuf_mutator

#undef MAX
#undef INITIAL_SIZE





