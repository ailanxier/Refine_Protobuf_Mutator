#include "mutator.h"
namespace protobuf_mutator {
    using std::placeholders::_1;
    // mutationType must contain None
    void restoreMutationBitset(MutationBitset& b) { b.reset(); b.set((int)FieldMuationType::None);}
    void restoreCrossoverBitset(CrossoverBitset& b) { b.reset(); b.set((int)CrossoverType::None);}
    inline string DebugEnumStr(FieldMuationType type){
        switch (type){
            case FieldMuationType::MutationAdd:
                return "      MutationAdd";
            case FieldMuationType::Delete:
                return "      Delete";
            case FieldMuationType::Mutate:
                return "      Mutate";
            case FieldMuationType::Shuffle:
                return "      Shuffle";
            default:
                return "      None";
        }
    }

    inline string DebugEnumStr(CrossoverType type){
        switch (type){
            case CrossoverType::Replace:
                return "      Replace";
            case CrossoverType::CrossoverAdd:
                return "      CrossoverAdd";
            default:
                return "      None";
        }
    }
    
    #define DEBUG_PRINT 0
    inline void printMutation(FieldMuationType mutationType, const FieldDescriptor* field, const Message* msg, int remain_size){
        if(DEBUG_PRINT){
            cout.fill(' ');
            string name;
            if(auto oneof = field->containing_oneof()){
                if(auto now_field = msg->GetReflection()->GetOneofFieldDescriptor(*msg, oneof)){
                    name = now_field->name();
                }else name = field->full_name() + "(undefine)";
            }else name = field->name();
            cout<<std::right<<std::setw(30)<<name<<" "<<DebugEnumStr(mutationType)<<" "<< remain_size<<std::endl;
        }
    }

    inline void printCrossover(CrossoverType crossoverType, const FieldDescriptor* field, const Message* msg, int remain_size){
        if(DEBUG_PRINT){
            cout.fill(' ');
            string name;
            if(auto oneof = field->containing_oneof()){
                if(auto now_field = msg->GetReflection()->GetOneofFieldDescriptor(*msg, oneof)){
                    name = now_field->name();
                }else name = field->full_name() + "(undefine)";
            }else name = field->name();
            cout<<std::right<<std::setw(30)<<name<<" "<<DebugEnumStr(crossoverType)<<" "<< remain_size<<std::endl;
        }
    }

    void Mutator::Mutate(Message* message, int& max_size) {
        int remain_size = max_size - GetMessageSize(message);
        // cout<<remain_size<<endl;
        MessageMutation(message, remain_size);
        max_size = remain_size;
    }

    void Mutator::MessageMutation(Message *msg, int& remain_size){
        # define MUTATION_ADD     allowed_mutations.set((int)FieldMuationType::MutationAdd)
        # define MUTATION_DELETE  allowed_mutations.set((int)FieldMuationType::Delete)
        # define MUTATION_MUTATE  allowed_mutations.set((int)FieldMuationType::Mutate)
        # define MUTATION_SHUFFLE allowed_mutations.set((int)FieldMuationType::Shuffle)
        # define TRY_MUTATE_FIELD TryMutateField(msg, field, allowed_mutations, remain_size)
        auto desc = msg->GetDescriptor();
        auto ref = msg->GetReflection();
        auto max_size = GetMessageSize(msg) + remain_size;
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
                        MUTATION_ADD;
                        TRY_MUTATE_FIELD;
                    }else{
                        // do not delete oneof field
                        // MUTATION_DELETE;
                        MUTATION_MUTATE;
                        TRY_MUTATE_FIELD;
                    }
                }
            }
            else if (field->is_repeated()) {
                MUTATION_ADD;
                MUTATION_DELETE;
                // Embedded message should be mutated recursively
                if(!IsMessageType(field)) MUTATION_MUTATE;
                MUTATION_SHUFFLE;
                TRY_MUTATE_FIELD;
                if(IsMessageType(field)){
                    int field_size = ref->FieldSize(*msg, field);
                    // Recursively processing all embedded message types
                    for(int i = 0; i < field_size; i++) 
                        MessageMutation(ref->MutableRepeatedMessage(msg, field, i), remain_size);
                }
            }else{
                if(IsMessageType(field))
                    MessageMutation(ref->MutableMessage(msg, field), remain_size);
                else if(ref->HasField(*msg, field)){
                    MUTATION_DELETE;
                    MUTATION_MUTATE;
                    TRY_MUTATE_FIELD;
                }else{
                    MUTATION_ADD;
                    TRY_MUTATE_FIELD;
                }
            }
            remain_size = max_size - msg->ByteSizeLong();
            // std::cout<<field->name()<<" "<<remain_size<<std::endl;
        }
    }

    void Mutator::TryMutateField(Message* msg, const FieldDescriptor* field, MutationBitset& allowed_mutations, int& remain_size){
        int tot = allowed_mutations.count();
        // select mutation type
        int order = GetRandomIndex(tot);
        int pos = allowed_mutations._Find_first();
        for(int i = 0; i < order; i++)
            pos = allowed_mutations._Find_next(pos);
        restoreMutationBitset(allowed_mutations);
        
        FieldMuationType mutationType = (FieldMuationType)pos;
        auto ref = msg->GetReflection();
        if(mutationType >= FieldMuationType::None) return;
        switch (mutationType){
            case FieldMuationType::MutationAdd:
                if(field->is_repeated())
                    AddRepeatedField(msg, field, remain_size);
                else
                    AddUnsetField(msg, field, remain_size);
                break;
            case FieldMuationType::Delete:
                if(field->is_repeated())
                    DeleteRepeatedField(msg, field, remain_size);
                else
                    DeleteSetField(msg, field, remain_size);
                break;
            case FieldMuationType::Mutate:
                if(field->is_repeated())
                    MutateRepeatedField(msg, field, remain_size);
                else{
                    if(auto oneof = field->containing_oneof()){
                        auto old_oneField = ref->GetOneofFieldDescriptor(*msg, oneof);
                        int old_index = old_oneField->index_in_oneof();
                        MutateSetField(msg, old_oneField, remain_size);
                        // If the index does not change and field is embedded message, then mutate the message itself 
                        if(old_index == ref->GetOneofFieldDescriptor(*msg, oneof)->index_in_oneof() && IsMessageType(old_oneField))
                            MessageMutation(ref->MutableMessage(msg, old_oneField), remain_size);
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
        printMutation(mutationType, field, msg, remain_size);
    }

    void Mutator::Crossover(Message* message1, const Message* message2, int& max_size) {
        int remain_size = max_size - GetMessageSize(message1);
        // cout<<remain_size<<endl;
        MessageCrossover(message1, message2, remain_size);
        max_size = remain_size;       
    }

    void Mutator::MessageCrossover(Message* msg1, const Message* msg2, int& remain_size){
        # define CROSSOVER_REPLACE              allowed_crossovers.set((int)CrossoverType::Replace)
        # define CROSSOVER_ADD                  allowed_crossovers.set((int)CrossoverType::CrossoverAdd)
        # define TRY_CROSSOVER_FIELD(f1, f2)    TryCrossoverField(msg1, msg2, f1, f2, allowed_crossovers, remain_size)
        # define NO_CROSSOVER                   allowed_crossovers.test((int)CrossoverType::None)
        auto desc1 = msg1->GetDescriptor();
        auto desc2 = msg2->GetDescriptor();
        auto ref1 = msg1->GetReflection();
        auto ref2 = msg2->GetReflection();
        auto max_size = GetMessageSize(msg1) + remain_size;
        CrossoverBitset allowed_crossovers;
        int field_count = desc1->field_count();
        for (int i = 0; i < field_count; i++) {
            restoreCrossoverBitset(allowed_crossovers);
            auto field1 = desc1->field(i);
            auto field2 = desc2->field(i);
            if (auto oneof1 = field1->containing_oneof()) {
                auto oneof2 = field2->containing_oneof();
                // Handle entire oneof group on the first field.
                if (field1->index_in_oneof() == 0) {
                    const FieldDescriptor* current_field1 = ref1->GetOneofFieldDescriptor(*msg1, oneof1);
                    const FieldDescriptor* current_field2 = ref2->GetOneofFieldDescriptor(*msg2, oneof2);
                    if(!current_field1){
                        if(current_field2){
                            CROSSOVER_ADD;
                            TRY_CROSSOVER_FIELD(field1, current_field2);
                        }
                    }else if(current_field2){
                        CROSSOVER_REPLACE;
                        TRY_CROSSOVER_FIELD(current_field1, current_field2);
                        // recursive crossover for embedded message type if do not replace
                        if(NO_CROSSOVER && IsMessageType(current_field1) && 
                            current_field1->index_in_oneof() == current_field2->index_in_oneof())
                            MessageCrossover(ref1->MutableMessage(msg1, current_field1), &ref2->GetMessage(*msg2, current_field2), remain_size);
                    }
                }
            }
            else if (field1->is_repeated()) {
                CROSSOVER_ADD;
                CROSSOVER_REPLACE;
                TRY_CROSSOVER_FIELD(field1, field2);
                // recursive crossover for embedded message type if do nothing
                if(NO_CROSSOVER && IsMessageType(field1)){
                    int field_size = min(ref1->FieldSize(*msg1, field1), ref2->FieldSize(*msg2, field2));
                    for(int i = 0; i < field_size; i++)
                        MessageCrossover(ref1->MutableRepeatedMessage(msg1, field1, i), 
                                        &ref2->GetRepeatedMessage(*msg2, field2, i), remain_size);
                }
            }else{
                if(IsMessageType(field1)){
                    MessageCrossover(ref1->MutableMessage(msg1, field1), &ref2->GetMessage(*msg2, field2), remain_size);
                }else if(ref2->HasField(*msg2, field2)){
                    if(ref1->HasField(*msg1, field1)){
                        CROSSOVER_REPLACE;
                        TRY_CROSSOVER_FIELD(field1, field2);
                    }else{
                        CROSSOVER_ADD;
                        TRY_CROSSOVER_FIELD(field1, field2);
                    }
                }
            }
            remain_size = max_size - GetMessageSize(msg1);
            // cout << field1->name() << " remain size: " << remain_size << endl;
        }
    }

    void Mutator::TryCrossoverField(Message* msg1, const Message* msg2, const FieldDescriptor* field1, const FieldDescriptor* field2, 
                                    CrossoverBitset& allowed_crossovers, int& remain_size){
        int tot = allowed_crossovers.count();
        int order = GetRandomIndex(tot);
        int pos = allowed_crossovers._Find_first();
        for(int i = 0; i < order; i++)
            pos = allowed_crossovers._Find_next(pos);
        CrossoverType crossoverType = (CrossoverType)pos;
        if(crossoverType >= CrossoverType::None)
            return;
        auto ref1 = msg1->GetReflection();
        auto ref2 = msg2->GetReflection();
        allowed_crossovers.reset();
        switch (crossoverType){
            case CrossoverType::Replace:
                if(field1->is_repeated())
                    ReplaceRepeatedField(msg1, msg2, field1, field2, remain_size);
                else
                    ReplaceSetField(msg1, msg2, field1, field2, remain_size);
                allowed_crossovers.set((int)CrossoverType::CrossoverAdd);
                break;
            case CrossoverType::CrossoverAdd:
                if(field1->is_repeated())
                    CrossoverAddRepeatedField(msg1, msg2, field1, field2, remain_size);
                else
                    CrossoverAddUnsetField(msg1, msg2, field1, field2, remain_size);
                allowed_crossovers.set((int)CrossoverType::Replace);
                break;
            default:
                allowed_crossovers.set((int)CrossoverType::None);
                break;
        }        
        printCrossover(crossoverType, field1, msg1, remain_size);
    }
    
    void Mutator::Seed(uint32_t value) {getRandEngine()->Seed(value); }
}  // namespace protobuf_mutator
