#include "mutator.h"
namespace protobuf_mutator {
    using std::placeholders::_1;
    void restoreMutationBitset(MutationBitset& b) { b.reset(); b.set((size_t)FieldMuationType::None);}
    void restoreCrossoverBitset(CrossoverBitset& b) { b.reset(); b.set((size_t)CrossoverType::None);}

    inline string DebugEnumStr(FieldMuationType type){
        switch (type){
            case FieldMuationType::Add:
                return "Add";
            case FieldMuationType::Delete:
                return "Delete";
            case FieldMuationType::Mutate:
                return "Mutate";
            case FieldMuationType::Shuffle:
                return "Shuffle";
            default:
                return "None";
        }
    }

    void Mutator::TryMutateField(Message* msg, const FieldDescriptor* field, MutationBitset& allowed_mutations, size_t& remain_size){
        int tot = allowed_mutations.count();
        int order = GetRandomIndex(tot);
        int pos = allowed_mutations._Find_first();
        for(int i = 0; i < order; i++)
            pos = allowed_mutations._Find_next(pos);
        restoreMutationBitset(allowed_mutations);
        
        FieldMuationType mutationType = (FieldMuationType)pos;
        auto ref = msg->GetReflection();
        std::cout<<field->name()<<" "<<DebugEnumStr(mutationType)<<std::endl;
        switch (mutationType){
            // 1. Add an unset field with random values.
            // 2. Add some new fields (including simple fields, enums, messages) with random values to a repeated field.
            case FieldMuationType::Add:
                if(field->is_repeated())
                    AddRepeatedField(msg, field, remain_size);
                else
                    AddUnsetField(msg, field, remain_size);
                break;
            // 1. Deletes a set field.
            // 2. Iterate each field in a repeated field and delete it with a certain probability
            case FieldMuationType::Delete:
                if(field->is_repeated())
                    DeleteRepeatedField(msg, field, remain_size);
                else
                    DeleteSetField(msg, field, remain_size);
                break;
            // Similar to the "delete" process, but byte mutation is used instead.
            case FieldMuationType::Mutate:
                if(field->is_repeated())
                    MutateRepeatedField(msg, field, remain_size);
                else{
                    if(auto oneof = field->containing_oneof()){
                        auto old_oneField = ref->GetOneofFieldDescriptor(*msg, oneof);
                        MutateSetField(msg, field, remain_size);
                        // If the index does not change, then mutate the field itself 
                        if(old_oneField == ref->GetOneofFieldDescriptor(*msg, oneof) && isMessageType(old_oneField))
                            messageMutation(ref->MutableMessage(msg, old_oneField), remain_size);
                    }else
                        MutateSetField(msg, field, remain_size);
                }
                break;
            case FieldMuationType::Shuffle:
                if(field->is_repeated())
                    ShuffleRepeatedField(msg, field, remain_size);
                break;
            default:
                break;
        }
    }
    
    void Mutator::messageMutation(Message *msg, size_t& remain_size){
        # define ADD     allowed_mutations.set((size_t)FieldMuationType::Add)
        # define DELETE  allowed_mutations.set((size_t)FieldMuationType::Delete)
        # define MUTATE  allowed_mutations.set((size_t)FieldMuationType::Mutate)
        # define SHUFFLE allowed_mutations.set((size_t)FieldMuationType::Shuffle)
        # define TRY_MUTATE_FIELD TryMutateField(msg, field, allowed_mutations, remain_size)
        auto desc = msg->GetDescriptor();
        auto ref = msg->GetReflection();
        MutationBitset allowed_mutations;
        restoreMutationBitset(allowed_mutations);
        int field_count = desc->field_count();
        for (int i = 0; i < field_count; i++) {
            auto field = desc->field(i);
            if (auto oneof = field->containing_oneof()) {
                // Handle entire oneof group on the first field.
                if (field->index_in_oneof() == 0) {
                    const FieldDescriptor* current_field = ref->GetOneofFieldDescriptor(*msg, oneof);
                    if(!current_field){
                        ADD;
                        TRY_MUTATE_FIELD;
                        continue;
                    }
                    DELETE;
                    MUTATE;
                    TRY_MUTATE_FIELD;
                }
                continue;
            }
            if (field->is_repeated()) {
                ADD;
                DELETE;
                // Embedded message should be mutated recursively
                if(!isMessageType(field)) MUTATE;
                SHUFFLE;
                TRY_MUTATE_FIELD;
                if(isMessageType(field)){
                    int field_size = ref->FieldSize(*msg, field);
                    for(int i = 0; i < field_size; i++)
                        messageMutation(ref->MutableRepeatedMessage(msg, field, i), remain_size);
                }
            }else{
                if(isMessageType(field)){
                    messageMutation(ref->MutableMessage(msg, field), remain_size);
                }else{
                    if(ref->HasField(*msg, field)){
                        DELETE;
                        MUTATE;
                        TRY_MUTATE_FIELD;
                    }else{
                        ADD;
                        TRY_MUTATE_FIELD;
                    }
                }
            }
        }
    }

    void Mutator::Mutate(Message* message, size_t& max_size) {
        size_t remain_size = max_size - message->ByteSizeLong();
        messageMutation(message, remain_size);
        max_size = remain_size;
    }

    void Mutator::Crossover(const Message& message1, Message* message2, size_t& max_size) {
        
    }
    
    void Mutator::Seed(uint32_t value) {getRandEngine()->Seed(value); }
}  // namespace protobuf_mutator
