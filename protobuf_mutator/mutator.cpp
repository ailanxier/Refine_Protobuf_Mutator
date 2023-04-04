#include "mutator.h"
namespace protobuf_mutator {

      using std::placeholders::_1;

namespace {

    bool IsProto3SimpleField(const FieldDescriptor& field) {return !field.is_repeated() && !field.has_presence();}

    struct CreateDefaultField : public FieldFunction<CreateDefaultField> {
        template <class T>
        void ForType(const FieldInstance& field) const {
        T value;
        field.GetDefault(&value);
        field.Create(value);
        }
    };

    struct DeleteField : public FieldFunction<DeleteField> {
        template <class T>
        void ForType(const FieldInstance& field) const {
        field.Delete();
        }
    };

    struct CopyField : public FieldFunction<CopyField> {
        template <class T>
        void ForType(const ConstFieldInstance& source, const FieldInstance& field) const {
            T value;
            source.Load(&value);
            field.Store(value);
        }
    };

    struct AppendField : public FieldFunction<AppendField> {
        template <class T>
        void ForType(const ConstFieldInstance& source, const FieldInstance& field) const {
            T value;
            source.Load(&value);
            field.Create(value);
        }
    };

    class CanCopyAndDifferentField : public FieldFunction<CanCopyAndDifferentField, bool> {
    public:
        template <class T>
        bool ForType(const ConstFieldInstance& src, const ConstFieldInstance& dst, int size_increase) const {
            T s;
            src.Load(&s);
            T d;
            dst.Load(&d);
            return SizeDiff(s, d) <= size_increase && !IsEqual(s, d);
        }

    private:
        bool IsEqual(const ConstFieldInstance::Enum& a, const ConstFieldInstance::Enum& b) const {
            assert(a.count == b.count);
            return a.index == b.index;
        }

        bool IsEqual(const std::unique_ptr<Message>& a, const std::unique_ptr<Message>& b) const {
            return MessageDifferencer::Equals(*a, *b);
        }

        template <class T>
        bool IsEqual(const T& a, const T& b) const { return a == b; }

        int64_t SizeDiff(const std::unique_ptr<Message>& src, const std::unique_ptr<Message>& dst) const {
            return src->ByteSizeLong() - dst->ByteSizeLong();
        }

        template <class T>
        int64_t SizeDiff(const T&, const T&) const { return 0; }
    };

    // Selects random field of compatible type to use for clone mutations.
    class DataSourceSampler {
    public:
        DataSourceSampler(const ConstFieldInstance& match, Random* random, int size_increase)
            : match_(match), random_(random), size_increase_(size_increase){}

        void Sample(const Message& message) { SampleImpl(message); }

        // Returns selected field.
        // const ConstFieldInstance& field() const {
        //   assert(!IsEmpty());
        //   return sampler_.selected();
        // }

        // bool IsEmpty() const { return sampler_.IsEmpty(); }

    private:
        ConstFieldInstance match_;
        Random* random_;
        int size_increase_;
        // WeightedReservoirSampler<ConstFieldInstance, Random> sampler_;

        void SampleImpl(const Message& message) {
        const Descriptor* descriptor = message.GetDescriptor();
        const Reflection* reflection = message.GetReflection();

        int field_count = descriptor->field_count();
        for (int i = 0; i < field_count; ++i) {
            const FieldDescriptor* field = descriptor->field(i);
            if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            if (field->is_repeated()) {
                const int field_size = reflection->FieldSize(message, field);
                for (int j = 0; j < field_size; ++j) {
                SampleImpl(reflection->GetRepeatedMessage(message, field, j));
                }
            } else if (reflection->HasField(message, field)) {
                SampleImpl(reflection->GetMessage(message, field));
            }
            }

            if (field->cpp_type() != match_.cpp_type()) continue;
            if (match_.cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
            if (field->enum_type() != match_.enum_type()) continue;
            } else if (match_.cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
            if (field->message_type() != match_.message_type()) continue;
            }

            if (field->is_repeated()) {
            if (int field_size = reflection->FieldSize(message, field)) {
                ConstFieldInstance source(&message, field, random_->randomIndex(field_size));
                // if (CanCopyAndDifferentField()(source, match_, size_increase_))
                // sampler_.Try(field_size, source);
            }
            } else {
            if (reflection->HasField(message, field)) {
                ConstFieldInstance source(&message, field);
                // if (CanCopyAndDifferentField()(source, match_, size_increase_))
                // sampler_.Try(1, source);
            }
            }
        }
        }
    };

    }  // namespace

    class FieldMutator {
    public:
        FieldMutator(int size_increase, bool enforce_changes, const ConstMessageVec& sources, Mutator* mutator)
        : size_increase_(size_increase), enforce_changes_(enforce_changes), sources_(sources), mutator_(mutator) {}

        void Mutate(int32_t* value) const { RepeatMutate(value, std::bind(&Mutator::MutateInt32, mutator_, _1)); }
        void Mutate(int64_t* value) const { RepeatMutate(value, std::bind(&Mutator::MutateInt64, mutator_, _1)); }
        void Mutate(uint32_t* value) const { RepeatMutate(value, std::bind(&Mutator::MutateUInt32, mutator_, _1)); }
        void Mutate(uint64_t* value) const { RepeatMutate(value, std::bind(&Mutator::MutateUInt64, mutator_, _1)); }
        void Mutate(float* value) const { RepeatMutate(value, std::bind(&Mutator::MutateFloat, mutator_, _1)); }
        void Mutate(double* value) const { RepeatMutate(value, std::bind(&Mutator::MutateDouble, mutator_, _1)); }
        void Mutate(bool* value) const { RepeatMutate(value, std::bind(&Mutator::MutateBool, mutator_, _1)); }
        void Mutate(FieldInstance::Enum* value) const {
            RepeatMutate(&value->index, std::bind(&Mutator::MutateEnum, mutator_, _1, value->count));
            assert(value->index < value->count);
        }
    private:
        int size_increase_;
        size_t enforce_changes_;
        const ConstMessageVec& sources_;
        Mutator* mutator_;
        
        template <class T, class F>
        void RepeatMutate(T* value, F mutate) const {
            if (!enforce_changes_ && mutator_->GetRandomIndex(mutator_->random_to_default_ratio_) == 0) 
                return;
            T tmp = *value;
            for (int i = 0; i < 10; ++i) {
                *value = mutate(*value);
                if (!enforce_changes_ || *value != tmp) return;
            }
        }
    };

    namespace {

    struct MutateField : public FieldFunction<MutateField> {
        template <class T>
        void ForType(const FieldInstance& field, int size_increase, const ConstMessageVec& sources, Mutator* mutator) const {
            T value;
            field.Load(&value);
            FieldMutator(size_increase, true, sources, mutator).Mutate(&value);
            field.Store(value);
        }
    };

    struct CreateField : public FieldFunction<CreateField> {
        public:
        template <class T>
        void ForType(const FieldInstance& field, int size_increase, const ConstMessageVec& sources, Mutator* mutator) const {
            T value;
            field.GetDefault(&value);
            /* defaults could be useful */
            FieldMutator field_mutator(size_increase, false, sources, mutator);
            field_mutator.Mutate(&value);
            field.Create(value);
        }
    };

    }  // namespace

    void Mutator::TryMutateField(Message* msg, FieldDescriptor* field, MutationBitset& allowed_mutations, int& remain_size){
        int tot = allowed_mutations.count();
        int order = GetRandomIndex(tot);
        int pos = allowed_mutations._Find_first();
        for(int i = 0; i < order; i++)
        pos = allowed_mutations._Find_next(pos);
        FieldMuationType mutationType = (FieldMuationType)pos;
        auto ref = msg->GetReflection();
    
        switch (mutationType){
            // 1. Add an unset field with random values.
            // 2. Add some new fields (including simple fields, enums, messages) with random values to a repeated field.
            case FieldMuationType::Add:
                if(field->is_repeated())
                AddRepeatedField(msg, field, remain_size, &random_);
                else
                AddUnsetField(msg, field, remain_size, &random_);
                break;
            // Delete a set fields
            case FieldMuationType::Delete:
                break;
            // Mutate a set fields
            case FieldMuationType::Mutate:

                break;
            case FieldMuationType::Shuffle:
                break;
            default:
                break;
        }
    }
    
    void Mutator::fieldMutation(Message *msg, int& remain_size){
        // const Descriptor* descriptor = msg->GetDescriptor();
        //   const Reflection* reflection = msg->GetReflection();

        //   int field_count = descriptor->field_count();
        //   for (int i = 0; i < field_count; ++i) {
        //     const FieldDescriptor* field = descriptor->field(i);
        //     if (const OneofDescriptor* oneof = field->containing_oneof()) {
        //       // Handle entire oneof group on the first field.
        //       if (field->index_in_oneof() == 0) {
        //         assert(oneof->field_count());
        //         const FieldDescriptor* current_field = reflection->GetOneofFieldDescriptor(*msg, oneof);
        //         for (;;) {
        //           const FieldDescriptor* add_field = oneof->field(GetRandomIndex(&random_, oneof->field_count()));
        //           if (add_field != current_field) {
        //             Try(msg, add_field, Mutation::Add, remain_size);
        //             Try(msg, add_field, Mutation::Clone, remain_size);
        //             break;
        //           }
        //           if (oneof->field_count() < 2) break;
        //         }
        //         if (current_field) {
        //           if (current_field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
        //             Try(msg, current_field, Mutation::Mutate, remain_size);
        //           Try(msg, current_field, Mutation::Delete, remain_size);
        //           Try(msg, current_field, Mutation::Copy, remain_size);
        //         }
        //       }
        //     } else {
        //       if (field->is_repeated()) {
        //         int field_size = reflection->FieldSize(*msg, field);
        //         size_t random_index = GetRandomIndex(&random_, field_size + 1);
        //         Try(msg, field, random_index, Mutation::Add, remain_size);
        //         Try(msg, field, random_index, Mutation::Clone, remain_size);

        //         if (field_size) {
        //           size_t random_index = GetRandomIndex(&random_, field_size);
        //           if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
        //             Try(msg, field, random_index, Mutation::Mutate, remain_size);
        //           Try(msg, field, random_index, Mutation::Delete, remain_size);
        //           Try(msg, field, random_index, Mutation::Copy, remain_size);
        //         }
        //       } else {
        //         if (reflection->HasField(*msg, field) || IsProto3SimpleField(*field)) {
        //           if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
        //             Try(msg, field, Mutation::Mutate, remain_size);
        //           if (!IsProto3SimpleField(*field) && (!field->is_required())) {
        //             Try(msg, field, Mutation::Delete, remain_size);
        //           }
        //           Try(msg, field, Mutation::Copy, remain_size);
        //         } else {
        //           Try(msg, field, Mutation::Add, remain_size);
        //           Try(msg, field, Mutation::Clone, remain_size);
        //         }
        //       }
        //     }

        //     if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        //       if (field->is_repeated()) {
        //         const int field_size = reflection->FieldSize(*msg, field);
        //         for (int j = 0; j < field_size; ++j)
        //           fieldMutation(reflection->MutableRepeatedMessage(msg, field, j), remain_size);
        //       }else if (reflection->HasField(*msg, field))
        //         fieldMutation(reflection->MutableMessage(msg, field), remain_size);
        //     }
        //   }
    }

    void Mutator::Seed(uint32_t value) { random_.Seed(value); }

    void Mutator::Mutate(Message* message, size_t max_size) { 
        messageVec messages;
        messages.push_back(message);

        ConstMessageVec sources(messages.begin(), messages.end());
        // MutateImpl(sources, messages, false, static_cast<int>(max_size) - static_cast<int>(message->ByteSizeLong()));
    }

    void Mutator::CrossOver(const Message& message1, Message* message2, size_t max_size) {
        messageVec messages;
        messages.push_back(message2);

        ConstMessageVec sources;
        sources.push_back(&message1);
        sources.push_back(message2);

        // MutateImpl(sources, messages, true, static_cast<int>(max_size) - static_cast<int>(message2->ByteSizeLong()));
    }

    int32_t Mutator::MutateInt32(int32_t value) { return rand(); }

    int64_t Mutator::MutateInt64(int64_t value) { return FlipBit(value); }

    uint32_t Mutator::MutateUInt32(uint32_t value) { return FlipBit(value);}

    uint64_t Mutator::MutateUInt64(uint64_t value) { return FlipBit(value); }

    float Mutator::MutateFloat(float value) { return FlipBit(value); }

    double Mutator::MutateDouble(double value) { return FlipBit(value); }

    bool Mutator::MutateBool(bool value) { return !value; }

    size_t Mutator::MutateEnum(size_t index, size_t item_count) {
        if (item_count <= 1) return 0;
        return (index + 1 + GetRandomIndex(item_count - 1)) % item_count;
    }
}  // namespace protobuf_mutator
