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

  class Mutator {
  public:
    // seed: value to initialize random number generator.
    Mutator() = default;
    virtual ~Mutator() = default;

    // Initialized internal random number generator.
    void Seed(uint32_t value);

    // message: message to mutate.
    // max_size_hint: approximate max ByteSize() of resulting message. Method does
    // not guarantee that real result will be strictly smaller than value. Caller
    // could repeat mutation if result was larger than expected.
    void Mutate(Message* message, size_t max_size_hint);

    void CrossOver(const Message& message1, Message* message2, size_t max_size_hint);

  protected:
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
    bool MutateImpl(const std::vector<const Message*>& sources, const std::vector<Message*>& messages, 
                    bool copy_clone_only, int size_increase_hint);
    bool keep_initialized_ = true;
    size_t random_to_default_ratio_ = 100;
    RandomEngine random_;
  };

}  // namespace protobuf_mutator

#endif  // SRC_MUTATOR_H_
