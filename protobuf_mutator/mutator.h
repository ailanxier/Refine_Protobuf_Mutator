#ifndef SRC_MUTATOR_H_
#define SRC_MUTATOR_H_

#include <unordered_map>
#include <bitset>
#include <iostream>
#include <map>
#include <utility>
#include "proto_util.h"
#include "field_instance.h"

namespace protobuf_mutator {
  
  /**
   * @brief Types of mutation on field in proto3
   * @details 
   * 1. Field include simple field, enum field, oneof field and repeated field.
   * 2. Oneof field can't be repeated and unset, so it can't be added or deleted (only mutated).
   * 3. Only simple field, enum field are considered for whether they are set or unset.
   *    Unset for simple field: has_xxx() returns false.
   * 4. An optional field is considered unset when it has no value, 
   *    while a singular field is considered unset when it has the default value. 
  */
  enum class FieldMuationType : uint8_t {
    // 1. Add an unset field with random values.
    // 2. Add some new fields (including simple fields, enums, messages) with random values to a repeated field.
    //    The total length of the newly added fields will not exceed half of the original length.
    Add   ,                
    // 1. Deletes a set field.
    // 2. Iterate each field in a repeated field and delete it with a certain probability
    Delete,                
    Mutate,                // Similar to the "delete" process, but byte mutation is used instead.
    Shuffle,               // Shuffle the order of the fields in a repeated field.
    None  ,                // Do nothing.
    END = None             // used to count the number of fieldMuationType.
  };

   /**
   * @brief Types of crossover on field in proto3
   * @details 
   *    After crossover the messages, do not perform recursive crossover on this message.
  */ 
  enum class CrossoverType : uint8_t {
    // 1. Replace some set fields (including embedded message) in message1 with the fields from message2.
    // 2. Replace some fields within a repeated field in message1 with message2.
    Replace  ,  
    // 1. Add some new simple fields from message2 to the corresponding unset fields in message1.
    // 2. Add some new fields from message2 to a repeated field in message1. 
    Add      ,     
    None     ,  // Do nothing. But recursive crossover for embedded message types.
    END = None  // used to count the number of crossoverType
  };
  using MutationBitset = bitset<static_cast<size_t>(FieldMuationType::END) + 1>;
  using CrossOverBitset = bitset<static_cast<size_t>(CrossoverType::END) + 1>;
  using messageVec = vector<Message*>;
  using ConstMessageVec = vector<const Message*>;
  class Mutator {
  public:
    // seed: value to initialize random number generator.
    Mutator() = default;
    virtual ~Mutator() = default;

    // Initialized internal random number generator.
    void Seed(uint32_t value);

    /**
     * @brief Mutate a message, and the result size does not exceed max_size
     * @details Three possible operations can be performed: Add£¬Delete, Mutate and Shuffle. 
     *          Refer to the definition of FieldMuationType.
     */
    void Mutate(Message* message, size_t max_size);

    /**
     * @brief Crossover message1 and message2, and save the resulting message in message1.
     *        The result size does not exceed max_size
     * @details Two possible operations can be performed: Replace and Add.
     *          Refer to the definition of CrossoverType.
     */
    void CrossOver(const Message& message1, Message* message2, size_t max_size);

  protected:
    // simple field mutation
    virtual int32_t MutateInt32(int32_t value);
    virtual int64_t MutateInt64(int64_t value);
    virtual uint32_t MutateUInt32(uint32_t value);
    virtual uint64_t MutateUInt64(uint64_t value);
    virtual float MutateFloat(float value);
    virtual double MutateDouble(double value);
    virtual bool MutateBool(bool value);
    virtual size_t MutateEnum(size_t index, size_t item_count);
    RandomEngine* random() { return &random_; }

  private:
    friend class FieldMutator;
    friend class TestMutator;
    void fieldMutation(Message* msg, int& remain_size);
    void TryMutateField(Message* msg, FieldDescriptor* field, MutationBitset& allowed_mutations, int& remain_size);
    bool keep_initialized_ = true;
    size_t random_to_default_ratio_ = 100;
    const uint64_t kDefaultMutateWeight = 1;
    RandomEngine random_;
  };
}  // namespace protobuf_mutator

#endif  // SRC_MUTATOR_H_
