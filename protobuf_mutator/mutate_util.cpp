#include "mutate_util.h"

namespace protobuf_mutator {
    namespace{
        int32_t MutateInt32(int32_t value) { return flipBit(value); }
        int64_t MutateInt64(int64_t value) { return flipBit(value); }
        uint32_t MutateUInt32(uint32_t value) { return flipBit(value);}
        uint64_t MutateUInt64(uint64_t value) { return flipBit(value); }
        float MutateFloat(float value) { return flipBit(value); }
        double MutateDouble(double value) { return flipBit(value); }
        bool MutateBool(bool value) { return !value; }
        
        template <class T, class F>
        void RepeatMutate(T* value, F mutate){
            T tmp = *value;
            for (int i = 0; i < 10; ++i) {
                *value = mutate(*value);
                if (*value != tmp) return;
            }
        }
        void mutate(int32_t* value) { RepeatMutate(value, std::bind(MutateInt32, _1)); }
        void mutate(int64_t* value) { RepeatMutate(value, std::bind(MutateInt64, _1)); }
        void mutate(uint32_t* value) { RepeatMutate(value, std::bind(MutateUInt32, _1)); }
        void mutate(uint64_t* value) { RepeatMutate(value, std::bind(MutateUInt64, _1)); }
        void mutate(float* value) { RepeatMutate(value, std::bind(MutateFloat, _1)); }
        void mutate(double* value) { RepeatMutate(value, std::bind(MutateDouble, _1)); }
        void mutate(bool* value) { RepeatMutate(value, std::bind(MutateBool, _1)); }
    }

    RandomEngine* getRandEngine(){
        static RandomEngine randEngine;
        return &randEngine;
    }

    void createRandomMessage(Message* msg, size_t& remain_size){
        auto desc = msg->GetDescriptor();
        auto ref = msg->GetReflection();
        auto field_count = desc->field_count();
        for (int i = 0; i < field_count; i++){
            auto field = desc->field(i);
            if (field->is_repeated())
                AddRepeatedField(msg, field, remain_size);
            else if(auto oneof_desc = field->containing_oneof()){
                // Handle entire oneof group on the first field.
                if(field->index_in_oneof() != 0) continue;
                auto new_field = oneof_desc->field(GetRandomIndex(oneof_desc->field_count() - 1));
                if(isMessageType(new_field)){
                    auto new_msg = ref->MutableMessage(msg, new_field)->New();
                    remain_size -= new_msg->ByteSizeLong();
                    createRandomMessage(new_msg, remain_size);
                    ref->SetAllocatedMessage(msg, new_msg, new_field);
                }else
                    AddUnsetField(msg, new_field, remain_size);
            }else
                AddUnsetField(msg, field, remain_size);
        }
    }

    void AddRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size){
        auto ref = msg->GetReflection();
        int oldLen = ref->FieldSize(*msg, field);
        // Temporarily proposed to add a length no more than MAX_NEW_REPEATED_SIZE
        int newLen = oldLen + GetRandomIndex(MAX_NEW_REPEATED_SIZE);
        auto max_size = msg->ByteSizeLong() + remain_size;
        for(int i = oldLen + 1; i <= newLen; i++){
            // add random field
            switch (field->cpp_type()){
                case FieldDescriptor::CPPTYPE_INT32:
                    ref->AddInt32(msg, field, GetRandomNum(INT32_MIN, INT32_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_INT64:
                    ref->AddInt64(msg, field, GetRandomNum(INT64_MIN, INT64_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_UINT32:
                    ref->AddUInt32(msg, field, GetRandomIndex(UINT32_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_UINT64:
                    ref->AddUInt64(msg, field, GetRandomIndex(UINT64_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_DOUBLE:
                    // small real number
                    ref->AddDouble(msg, field, GetRandomNum(-1.0, 1.0));
                    break;
                case FieldDescriptor::CPPTYPE_FLOAT:
                    ref->AddFloat(msg, field, GetRandomNum(-1.0, 1.0));
                    break;
                case FieldDescriptor::CPPTYPE_BOOL:
                    ref->AddBool(msg, field, GetRandomIndex(1));
                    break;
                case FieldDescriptor::CPPTYPE_ENUM:
                    ref->AddEnum(msg, field, field->enum_type()->value(GetRandomIndex(field->enum_type()->value_count() - 1)));
                    break;
                case FieldDescriptor::CPPTYPE_MESSAGE:{
                    auto newMsg = ref->AddMessage(msg, field);
                    size_t msg_max_size = max_size - msg->ByteSizeLong();
                    createRandomMessage(newMsg, msg_max_size);
                    break;
                }
                case FieldDescriptor::CPPTYPE_STRING:
                    // not support
                    break;
            }
            // check size
            remain_size = max_size - msg->ByteSizeLong();
            if(remain_size < 0) {
                ref->RemoveLast(msg, field);
                remain_size = max_size - msg->ByteSizeLong();
                return;
            }      
        }
    }

    void AddUnsetField(Message* msg, const FieldDescriptor* field, size_t& remain_size){
        auto ref = msg->GetReflection();
        auto max_size = msg->ByteSizeLong() + remain_size;
        // Special judgment for oneof fields
        if(auto oneof_desc = field->containing_oneof()){
            auto new_field = oneof_desc->field(GetRandomIndex(oneof_desc->field_count() - 1));
            if(isMessageType(new_field)){
                auto new_msg = ref->MutableMessage(msg, new_field)->New();
                remain_size -= new_msg->ByteSizeLong();
                createRandomMessage(new_msg, remain_size);
                ref->SetAllocatedMessage(msg, new_msg, new_field);
                return;
            }
            else field = new_field;
        }
        switch (field->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:
                ref->SetInt32(msg, field, GetRandomNum(INT32_MIN, INT32_MAX));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                ref->SetInt64(msg, field, GetRandomNum(INT64_MIN, INT64_MAX));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                ref->SetUInt32(msg, field, GetRandomIndex(UINT32_MAX));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                ref->SetUInt64(msg, field, GetRandomIndex(UINT64_MAX));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                // small real number
                ref->SetDouble(msg, field, GetRandomNum(-1.0, 1.0));
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                ref->SetFloat(msg, field, GetRandomNum(-1.0, 1.0));
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                ref->SetBool(msg, field, GetRandomIndex(1));
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                ref->SetEnum(msg, field, field->enum_type()->value(GetRandomIndex(field->enum_type()->value_count() - 1)));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:{
                auto newMsg = ref->MutableMessage(msg, field);
                size_t msg_max_size = max_size - msg->ByteSizeLong();
                createRandomMessage(newMsg, msg_max_size);
                break;
            }case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        // check size
        remain_size = max_size - msg->ByteSizeLong();
        if(remain_size < 0) {
            ref->ClearField(msg, field);
            remain_size = max_size - msg->ByteSizeLong();
        }
    }

    void DeleteRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size){
        auto ref = msg->GetReflection();
        auto type = field->cpp_type();
        int len = ref->FieldSize(*msg, field);
        auto max_size = msg->ByteSizeLong() + remain_size;
        // O (n ^ 2) implementation
        for(int i = len - 1; i >= 0; i--){
            if(!canDelete()) continue;
            for(int j = i; j + 1 < len; j++)
                ref->SwapElements(msg, field, j, j + 1);
            ref->RemoveLast(msg, field);
            len--;
        }
        remain_size = max_size - msg->ByteSizeLong();
    }
    
    void DeleteSetField(Message* msg, const FieldDescriptor* field, size_t& remain_size){
        //if(randomIndex(DELETE_PROBABILITY) != 0) return;
        auto ref = msg->GetReflection();
        auto type = field->cpp_type();
        auto max_size = msg->ByteSizeLong() + remain_size;
        // Special judgment for oneof fields
        if(auto oneof_desc = field->containing_oneof())
            ref->ClearOneof(msg, oneof_desc);
        else
            ref->ClearField(msg, field);
        
        remain_size = max_size - msg->ByteSizeLong();
    }

    void MutateRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size){
        auto ref = msg->GetReflection();
        int len = ref->FieldSize(*msg, field);
        auto max_size = msg->ByteSizeLong() + remain_size;
        switch (field->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;
                    auto now = ref->GetRepeatedInt32(*msg, field, i);
                    mutate(&now);
                    ref->SetRepeatedInt32(msg, field, i, now);
                }
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;                    
                    auto now = ref->GetRepeatedInt64(*msg, field, i);
                    mutate(&now);
                    ref->SetRepeatedInt64(msg, field, i, now);
                }
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;
                    auto now = ref->GetRepeatedUInt32(*msg, field, i);
                    mutate(&now);
                    ref->SetRepeatedUInt32(msg, field, i, now);
                }
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;
                    auto now = ref->GetRepeatedUInt64(*msg, field, i);
                    mutate(&now);
                    ref->SetRepeatedUInt64(msg, field, i, now);
                }
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;
                    auto now = ref->GetRepeatedDouble(*msg, field, i);
                    mutate(&now);
                    ref->SetRepeatedDouble(msg, field, i, now);
                }
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;
                    auto now = ref->GetRepeatedFloat(*msg, field, i);
                    mutate(&now);
                    ref->SetRepeatedFloat(msg, field, i, now);
                }
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;
                    auto now = ref->GetRepeatedBool(*msg, field, i);
                    mutate(&now);
                    ref->SetRepeatedInt32(msg, field, i, now);
                }
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                for(int i = 0; i < len; i++){
                    if(!canMutate()) continue;
                    auto now = ref->GetRepeatedEnumValue(*msg, field, i);
                    mutate(&now);
                    now %= field->enum_type()->value_count();
                    ref->SetRepeatedEnumValue(msg, field, i, now);
                }              
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:
            case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        remain_size = max_size - msg->ByteSizeLong();
    }

    void MutateSetField(Message* msg, const FieldDescriptor* field, size_t& remain_size){
        auto ref = msg->GetReflection();
        auto max_size = msg->ByteSizeLong() + remain_size;
        // Special judgment for oneof fields
        if(auto oneof_desc = field->containing_oneof()){
            int index = ref->GetOneofFieldDescriptor(*msg, oneof_desc)->index();
            int newIndex = index;
            mutate(&newIndex);
            newIndex %= oneof_desc->field_count();
            // If the index does not change, then mutate the field itself
            if(index == newIndex) return;
            auto new_field = oneof_desc->field(newIndex);
            if(isMessageType(new_field)){
                auto new_msg = ref->MutableMessage(msg, new_field)->New();
                // TODO: Unable to get the byte size of the field
                createRandomMessage(new_msg, remain_size);
                ref->SetAllocatedMessage(msg, new_msg, new_field);
                remain_size = max_size - msg->ByteSizeLong();
                return;
            }
            else field = new_field; 
        }
        switch (field->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:{
                auto now = ref->GetInt32(*msg, field);
                mutate(&now);
                ref->SetInt32(msg, field, now);
                break;
            } case FieldDescriptor::CPPTYPE_INT64:{
                auto now = ref->GetInt64(*msg, field);
                mutate(&now);
                ref->SetInt64(msg, field, now);
                break;
            } case FieldDescriptor::CPPTYPE_UINT32:{
                auto now = ref->GetUInt32(*msg, field);
                mutate(&now);
                ref->SetUInt32(msg, field, now);
                break;
            } case FieldDescriptor::CPPTYPE_UINT64:{
                auto now = ref->GetUInt64(*msg, field);
                mutate(&now);
                ref->SetUInt64(msg, field, now);
                break;
            } case FieldDescriptor::CPPTYPE_DOUBLE:{
                auto now = ref->GetDouble(*msg, field);
                mutate(&now);
                ref->SetDouble(msg, field, now);
                break;
            } case FieldDescriptor::CPPTYPE_FLOAT:{
                auto now = ref->GetFloat(*msg, field);
                mutate(&now);
                ref->SetFloat(msg, field, now);
                break;
            } case FieldDescriptor::CPPTYPE_BOOL:{
                auto now = ref->GetBool(*msg, field);
                mutate(&now);
                ref->SetBool(msg, field, now);
                break;
            } case FieldDescriptor::CPPTYPE_ENUM:{
                auto now = ref->GetEnumValue(*msg, field);
                mutate(&now);
                now %= field->enum_type()->value_count();
                ref->SetEnumValue(msg, field, now);                        
                break;
            } 
            case FieldDescriptor::CPPTYPE_MESSAGE:
            case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        remain_size = max_size - msg->ByteSizeLong();
    }

    void ShuffleRepeatedField(Message* msg, const FieldDescriptor* field, size_t& remain_size){
        auto ref = msg->GetReflection();
        auto max_size = msg->ByteSizeLong() + remain_size;
        auto len = ref->FieldSize(*msg, field);
        switch (field->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:{
                vector<int32_t> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedInt32(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedInt32(msg, field, i, shuffleList[i]); 
                break;
            } case FieldDescriptor::CPPTYPE_INT64:{
                vector<int64_t> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedInt64(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedInt64(msg, field, i, shuffleList[i]); 
                break;
            } case FieldDescriptor::CPPTYPE_UINT32:{
                vector<uint32_t> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedUInt32(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedUInt32(msg, field, i, shuffleList[i]); 
                break;
            } case FieldDescriptor::CPPTYPE_UINT64:{
                vector<uint64_t> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedUInt64(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedUInt64(msg, field, i, shuffleList[i]); 
                break;
            } case FieldDescriptor::CPPTYPE_DOUBLE:{
                vector<double> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedDouble(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedDouble(msg, field, i, shuffleList[i]); 
                break;
            } case FieldDescriptor::CPPTYPE_FLOAT:{
                vector<float> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedFloat(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedFloat(msg, field, i, shuffleList[i]); 
                break;
            } case FieldDescriptor::CPPTYPE_BOOL:{
                vector<bool> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedBool(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedBool(msg, field, i, shuffleList[i]); 
                break;
            } case FieldDescriptor::CPPTYPE_ENUM:{
                vector<int> shuffleList(len);
                for(int i = 0; i < len; i++) shuffleList[i] = ref->GetRepeatedEnumValue(*msg, field, i);
                shuffle(shuffleList.begin(), shuffleList.end(), getRandEngine()->randLongEngine);
                for(int i = 0; i < len; i++) ref->SetRepeatedEnumValue(msg, field, i, shuffleList[i]);
                break;
            } case FieldDescriptor::CPPTYPE_MESSAGE:{
                // Due to the limitations of the protobuf API, the only way to shuffle 
                // the order of embedded message repeated fields is to randomly swap two messages
                if(len == 1) return;
                for(int i = 0; i < len; i++){
                    auto idx1 = GetRandomIndex(len - 1);
                    auto idx2 = GetRandomIndex(len - 1);
                    ref->SwapElements(msg, field, idx1, idx2);
                }
                break;
            } case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        remain_size = max_size - msg->ByteSizeLong();
    }

}