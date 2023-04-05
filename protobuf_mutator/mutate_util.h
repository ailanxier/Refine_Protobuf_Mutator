#ifndef SRC_MUTATE_UTIL_H_
#define SRC_MUTATE_UTIL_H_

#include "proto_util.h"
namespace protobuf_mutator {
    #define MAX_NEW_REPEATED_SIZE 5
    // 1 / DELETE_PROBABILITY
    #define DELETE_PROBABILITY 3 
    
    #define MUTATE_PROBABILITY 3 
    
    using std::min;
    using std::placeholders::_1;
    using std::vector;
    using std::shuffle;
    class RandomEngine{
    public:
        void Seed(unsigned int seed) {
            seed_ = seed;
            randLongEngine.seed(seed);
            randomDoubleEngine.seed(seed);
        }
        unsigned int seed_;
        std::minstd_rand randLongEngine;
        std::random_device randDevice;
        std::mt19937 randomDoubleEngine;
    };

    RandomEngine* getRandEngine();

    /**
     * @brief Get a random number within the range [mi, ma].
     */      
    template <typename T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type 
    GetRandomNum(T mi, T ma){
        assert(mi <= ma);
        std::uniform_int_distribution<T> distribution(mi, ma);
        return distribution(getRandEngine()->randLongEngine);
    }

    template <typename T>
    inline typename std::enable_if<std::is_floating_point<T>::value, T>::type 
    GetRandomNum(T mi, T ma){
        assert(mi <= ma);
        std::uniform_real_distribution<T> distribution(mi, ma);
        return distribution(getRandEngine()->randomDoubleEngine);
    }

    /**
     * @brief Get a random integer within the range [0, ma] as an array index.
     */
    template <typename T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    GetRandomIndex(T ma){
        assert(ma >= 0);
        std::uniform_int_distribution<T> distribution(0, ma);
        return distribution(getRandEngine()->randLongEngine);
    }

    inline void flipBit(size_t size, uint8_t* bytes) {
        size_t bit = GetRandomIndex(size * 8);
        bytes[bit / 8] ^= (1u << (bit % 8));
    }

    /**
     * @brief Flips random bit in the value.
     */    
    template <class T>
    inline T flipBit(T value) {
        flipBit(sizeof(value), reinterpret_cast<uint8_t*>(&value));
        return value;
    }
    inline bool canMutate() { return GetRandomNum(1, MUTATE_PROBABILITY) == 1;}
    inline bool canDelete() { return GetRandomNum(1, DELETE_PROBABILITY) == 1;}
    inline bool isMessageType(const FieldDescriptor* field) {return field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE;}
    // recursive create a message with random value
    void createRandomMessage(Message* msg, size_t& remain_size);
    // Add some new fields (including simple fields, enums, messages) with random values to a repeated field.
    void AddRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size);
    // Add an unset field with random values.
    void AddUnsetField(Message* msg, const FieldDescriptor* field, size_t& remain_size);
    // Iterate each field in a repeated field and delete it with a certain probability
    void DeleteRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size);   
    // Deletes a set field
    void DeleteSetField(Message* msg, const FieldDescriptor* field, size_t& remain_size);
    // Similar to the "delete" process, but byte mutation is used instead. 
    // Not used for mutate embedded message.
    void MutateRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size);
    void MutateSetField(Message* msg, const FieldDescriptor* field, size_t& remain_size);
    // Shuffle the order of the fields in a repeated field.
    void ShuffleRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size);

}  // namespace protobuf_mutator

#endif  // SRC_MUTATE_UTIL_H_

