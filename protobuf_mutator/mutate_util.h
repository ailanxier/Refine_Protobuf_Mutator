#ifndef SRC_MUTATE_UTIL_H_
#define SRC_MUTATE_UTIL_H_

#include "proto_util.h"

namespace protobuf_mutator {
    #define MAX_NEW_REPEATED_SIZE 5
    using std::min;
    class Random{
    public:
        void Seed(unsigned int seed) {
            randLongEngine.seed(seed);
            randomDoubleEngine.seed(seed);
        }
        template <typename T>
        typename std::enable_if<std::is_integral<T>::value, T>::type 
        randomNum(T mi, T ma){
            assert(mi <= ma);
            std::uniform_int_distribution<T> distribution(mi, ma);
            return distribution(randLongEngine);
        }

        template <typename T>
        typename std::enable_if<std::is_floating_point<T>::value, T>::type 
        randomNum(T mi, T ma){
            assert(mi <= ma);
            std::uniform_real_distribution<T> distribution(mi, ma);
            return distribution(randomDoubleEngine);
        }

        template <typename T>
        typename std::enable_if<std::is_integral<T>::value, T>::type
        randomIndex(T ma){
            assert(ma >= 0);
            std::uniform_int_distribution<T> distribution(0, ma);
            return distribution(randLongEngine);
        }

        void flipBit(size_t size, uint8_t* bytes) {
            size_t bit = randomIndex(size * 8);
            bytes[bit / 8] ^= (1u << (bit % 8));
        }

        template <class T>
        T flipBit(T value) {
            flipBit(sizeof(value), reinterpret_cast<uint8_t*>(&value));
            return value;
        }

        unsigned int seed_;
        std::minstd_rand randLongEngine;
        std::random_device randDevice;
        std::mt19937 randomDoubleEngine;
    };

    // recursive create a message with random value
    void createRandomMessage(Message* msg, int& remain_size, Random* rand);
    void AddRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size, Random* rand);
    void AddUnsetField(Message* msg, const FieldDescriptor* field, int& remain_size, Random* rand);
    void DeleteRepeatedField(Message* msg, const FieldDescriptor* field, Random* rand);
    void DeleteSetField(Message* msg, const FieldDescriptor* field, Random* rand);
}  // namespace protobuf_mutator

#endif  // SRC_MUTATE_UTIL_H_
