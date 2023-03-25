#include "afl_mutator.h"
#include <random>
#include <algorithm>

#define INITIAL_SIZE (100)
#define MAX(x, y) ( ((x) > (y)) ? (x) : y )

namespace protobuf_mutator{
    namespace aflplusplus {
        namespace {
            size_t GetRandom(size_t s){
                static std::default_random_engine generator(s);
                std::uniform_int_distribution<size_t> uniform_dist(1, 10);
                return uniform_dist(generator);
            }
        } // namespace

        MutateHelper::MutateHelper(size_t s) {
            buf_  = static_cast<uint8_t*>(calloc(1, INITIAL_SIZE));
            if(!buf_) perror("MutateHelper init");
            seed_ = s;
        }

        uint8_t* MutateHelper::ReallocBuf(size_t length) {
            len_ = MAX(length, INITIAL_SIZE);
            buf_ = static_cast<uint8_t*>(realloc(buf_, len_));
            if(!buf_) perror("MutateHelper realloc");
            return buf_;
        }

        size_t AFL_CustomProtoMutator(MutateHelper *m, bool binary, unsigned char *buf, size_t buf_size, 
                                    unsigned char **out_buf, unsigned char *add_buf, 
                                    size_t add_buf_size, size_t max_size, 
                                    protobuf::Message* input1, protobuf::Message* input2) {                   
            // (PC) probability of crossover : 0.8 (0.6~1.0)
            // (PM) probability of mutation  : 0.2 
            size_t now = GetRandom(m->GetSeed());
            // std::ofstream random_of("random.txt", std::ios::out | std::ios::app);
            // random_of << now << std::endl;
            // random_of.close();
            if(now <= 2){
                memcpy(m->ReallocBuf(std::max(max_size, buf_size)), buf, buf_size);
                m->SetLen(CustomProtoMutator(binary, m->GetOutBuf(), buf_size, max_size, m->GetSeed(), input1));
            }else{
                // Crossover buf and add_buf and store the outbuf to m.buf_
                m->ReallocBuf(std::max({max_size, buf_size, add_buf_size}));
                m->SetLen(CustomProtoCrossOver(binary, buf, buf_size, add_buf, add_buf_size, m->GetOutBuf(), 
                                                max_size, m->GetSeed(), input1, input2));
            }
            *out_buf = m->GetOutBuf();
            return m->GetLen();
        }

    } // aflplusplus
} // protobuf_mutator

#undef MAX
#undef INITIAL_SIZE





