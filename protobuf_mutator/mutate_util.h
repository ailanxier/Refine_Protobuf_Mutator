#pragma once

#include "proto_util.h"
namespace protobuf_mutator {
    #define MAX_NEW_REPEATED_SIZE 5
    #define MAX_REPLACE_REPEATED_SIZE 5
    // 1 / DELETE_PROBABILITY
    #define DELETE_REPEATED_FIELD_PROBABILITY 4
    #define DELETE_SIMPLE_FIELD_PROBABILITY 2 
    #define MUTATE_PROBABILITY 3 
    
    using std::min;
    using std::placeholders::_1;
    using std::vector;
    using std::shuffle;
    using std::cout;
    using std::endl;
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
        // assert(mi <= ma);
        // do not use assert to avoid stopping fuzzing
        if(mi > ma) return mi;
        std::uniform_int_distribution<T> distribution(mi, ma);
        return distribution(getRandEngine()->randLongEngine);
    }

    template <typename T>
    inline typename std::enable_if<std::is_floating_point<T>::value, T>::type 
    GetRandomNum(T mi, T ma){
        // assert(mi <= ma);
        if(mi > ma) return mi;
        std::uniform_real_distribution<T> distribution(mi, ma);
        return distribution(getRandEngine()->randomDoubleEngine);
    }

    /**
     * @brief Get a random integer within the range [0, ma] as an array index.
     */
    template <typename T>
    inline typename std::enable_if<std::is_integral<T>::value, T>::type
    GetRandomIndex(T ma){
        // assert(ma >= 0);
        if(ma < 0) return 0;
        std::uniform_int_distribution<T> distribution(0, ma);
        return distribution(getRandEngine()->randLongEngine);
    }

    inline void flipBit(size_t size, uint8_t* bytes) {
        size_t bit = GetRandomIndex(size * 8 - 1);
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
    inline bool CanMutate() { return GetRandomNum(1, MUTATE_PROBABILITY) == 1;}
    inline bool CanDeleteRepeatedField() { return GetRandomNum(1, DELETE_REPEATED_FIELD_PROBABILITY) == 1;}
    inline bool CanDeleteSimpleField() { return GetRandomNum(1, DELETE_SIMPLE_FIELD_PROBABILITY) == 1;}
    inline bool IsMessageType(const FieldDescriptor* field) {return field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE;}
    // XXX: only for small message
    inline int  GetMessageSize(const Message* msg) {return (int)msg->ByteSizeLong();}
    template<typename T>
    inline T NotNegMod(T a, const T mod) { return (a % mod + mod) % mod; }

    // ----------------------Mutate functions------------------------
    // recursive create a message with random value
    void createRandomMessage(Message* msg, int& remain_size);
    // Add some new fields (including simple fields, enums, messages) with random values to a repeated field.
    void AddRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size, int min_new_size = 1);
    // Add an unset field with random values.
    void AddUnsetField(Message* msg, const FieldDescriptor* field, int& remain_size);
    // Iterate each field in a repeated field and delete it with a certain probability
    void DeleteRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size);   
    // Deletes a set field
    void DeleteSetField(Message* msg, const FieldDescriptor* field, int& remain_size);
    // Similar to the "delete" process, but byte mutation is used instead. 
    // Not used for mutate embedded message.
    void MutateRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size);
    void MutateSetField(Message* msg, const FieldDescriptor* field, int& remain_size);
    // Shuffle the order of the fields in a repeated field.
    void ShuffleRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size);

    // ----------------------Crossover functions----------------------
    // Replace some fields within a repeated field in message1 with message2.
    void ReplaceRepeatedField(Message* msg1, const Message* msg2, const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size);
    // Replace some set fields in message1 with the fields from message2.
    void ReplaceSetField(Message* msg1, const Message* msg2, const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size);
    // Add some new fields from message2 to a repeated field in message1.
    void CrossoverAddRepeatedField(Message* msg1, const Message* msg2, const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size);
    // Add some new simple fields from message2 to the corresponding unset fields in message1.
    void CrossoverAddUnsetField(Message* msg1, const Message* msg2, const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size);
}  // namespace protobuf_mutator