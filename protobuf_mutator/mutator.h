#ifndef SRC_MUTATOR_H_
#define SRC_MUTATOR_H_

#include <unordered_map>
#include <bitset>
#include <iostream>
#include <map>
#include <utility>
#include <iomanip>
#include "proto_util.h"
#include "mutate_util.h"

namespace protobuf_mutator {

    /**
     * @brief Types of mutation on field in proto3
     * @details 
     * 1. Field include simple field, enum field, oneof field and repeated field.
     * 2. Simple field, enum field, oneof field need to be checked whether they are set or unset.
     *    Unset for simple field: has_xxx() returns false and DebugString() shows nothing.
     * 3. An optional field is considered unset when it has no value, 
     *    while a singular field is considered unset when it has the default value. 
     * 4. Oneof field needs to be specially handled, and it can't be repeated.
     */
    enum class FieldMuationType : uint8_t {
        // 1. Add an unset field with random values.
        // 2. Add some new fields (including simple fields, enums, oneof) with random values to a repeated field.
        //    The total length of the newly added fields will not exceed MAX_NEW_REPEATED_SIZE.
        MutationAdd  ,                
        // 1. Deletes a set field.
        // 2. Iterate each field in a repeated field and delete it with a certain probability
        Delete       ,                
        // Similar to the "delete" process, but byte mutation is used instead. 
        // Not used for mutate embedded message.
        Mutate       ,                
        Shuffle      ,         // Shuffle the order of the fields in a repeated field.
        None         ,         // Do nothing.
        END = None             // used to count the number of fieldMuationType.
    };

    /**
     * @brief Types of crossover on field in proto3
     * @details 
     *    After crossover the messages, do not perform recursive crossover on this message.
     */ 
    enum class CrossoverType : uint8_t {
        // 1. Replace some set fields in message1 with the fields from message2.
        // 2. Replace some fields within a repeated field in message1 with message2.
        Replace     ,  
        // 1. Add some new simple fields from message2 to the corresponding unset fields in message1.
        // 2. Add some new fields from message2 to a repeated field in message1. 
        CrossoverAdd,     
        None        ,  // Do nothing. But recursive crossover for embedded message types.
        END = None     // used to count the number of crossoverType
    };
    using MutationBitset = bitset<static_cast<int>(FieldMuationType::END) + 1>;
    using CrossoverBitset = bitset<static_cast<int>(CrossoverType::END) + 1>;
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
        void Mutate(Message* message, int& max_size);

        /**
         * @brief Crossover message1 and message2, and save the resulting message in message1.
         *        The result size does not exceed max_size
         * @details Two possible operations can be performed: Replace and Add.
         *          Refer to the definition of CrossoverType.
         */
        void Crossover(Message* message1, const Message* message2, int& max_size);

    private:
        void MessageMutation(Message* msg, int& remain_size);
        void TryMutateField(Message* msg, const FieldDescriptor* field, MutationBitset& allowed_mutations, int& remain_size);
        void MessageCrossover(Message* msg1, const Message* msg2, int& remain_size);
        void TryCrossoverField(Message* msg1, const Message* msg2, const FieldDescriptor* field1, const FieldDescriptor* field2, CrossoverBitset& allowed_crossovers, int& remain_size);
    };
}  // namespace protobuf_mutator

#endif  // SRC_MUTATOR_H_
