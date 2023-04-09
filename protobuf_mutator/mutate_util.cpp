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

    void createRandomMessage(Message* msg, int& remain_size){
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
                if(IsMessageType(new_field)){
                    auto new_msg = ref->GetMessage(*msg, new_field).New();
                    remain_size -= GetMessageSize(new_msg);
                    createRandomMessage(new_msg, remain_size);
                    ref->SetAllocatedMessage(msg, new_msg, new_field);
                }else
                    AddUnsetField(msg, new_field, remain_size);
            }else
                AddUnsetField(msg, field, remain_size);
        }
    }

    void AddRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size){
        auto ref = msg->GetReflection();
        auto oldLen = ref->FieldSize(*msg, field);
        // newLen in [1, MAX_NEW_REPEATED_SIZE]
        auto newLen = GetRandomNum(1, MAX_NEW_REPEATED_SIZE);
        auto max_size = GetMessageSize(msg) + remain_size;
        for(int i = 1; i <= newLen; i++){
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
                    int msg_max_size = max_size - GetMessageSize(msg);
                    createRandomMessage(newMsg, msg_max_size);
                    break;
                }
                case FieldDescriptor::CPPTYPE_STRING:
                    // not support
                    break;
            }
            // check size
            remain_size = max_size - GetMessageSize(msg);
            if(remain_size < 0) {
                ref->RemoveLast(msg, field);
                remain_size = max_size - GetMessageSize(msg);
                return;
            }      
        }
    }

    void AddUnsetField(Message* msg, const FieldDescriptor* field, int& remain_size){
        auto ref = msg->GetReflection();
        auto max_size = GetMessageSize(msg) + remain_size;
        // Special judgment for oneof fields
        if(auto oneof_desc = field->containing_oneof()){
            auto new_field = oneof_desc->field(GetRandomIndex(oneof_desc->field_count() - 1));
            if(IsMessageType(new_field)){
                auto new_msg = ref->GetMessage(*msg, new_field).New();
                remain_size -= GetMessageSize(new_msg);
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
                int msg_max_size = max_size - GetMessageSize(msg);
                createRandomMessage(newMsg, msg_max_size);
                break;
            }case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        // check size
        remain_size = max_size - GetMessageSize(msg);
        if(remain_size < 0) {
            ref->ClearField(msg, field);
            remain_size = max_size - GetMessageSize(msg);
        }
    }

    void DeleteRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size){
        auto ref = msg->GetReflection();
        auto type = field->cpp_type();
        auto len = ref->FieldSize(*msg, field);
        auto max_size = GetMessageSize(msg) + remain_size;
        // O (n ^ 2) implementation
        for(int i = len - 1; i >= 0; i--){
            if(!CanDelete()) continue;
            for(int j = i; j + 1 < len; j++)
                ref->SwapElements(msg, field, j, j + 1);
            ref->RemoveLast(msg, field);
            len--;
        }
        remain_size = max_size - GetMessageSize(msg);
    }
    
    void DeleteSetField(Message* msg, const FieldDescriptor* field, int& remain_size){
        if(!CanDelete()) return;
        auto ref = msg->GetReflection();
        auto type = field->cpp_type();
        auto max_size = GetMessageSize(msg) + remain_size;
        // Special judgment for oneof fields
        if(auto oneof_desc = field->containing_oneof())
            ref->ClearOneof(msg, oneof_desc);
        else
            ref->ClearField(msg, field);
        
        remain_size = max_size - GetMessageSize(msg);
    }

    void MutateRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size){
        auto ref = msg->GetReflection();
        auto len = ref->FieldSize(*msg, field);
        auto max_size = GetMessageSize(msg) + remain_size;
        switch (field->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;
                    auto now = ref->GetRepeatedInt32(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    ref->SetRepeatedInt32(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedInt32(msg, field, i, temp);
                }
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;                    
                    auto now = ref->GetRepeatedInt64(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    ref->SetRepeatedInt64(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedInt64(msg, field, i, temp);
                }
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;
                    auto now = ref->GetRepeatedUInt32(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    ref->SetRepeatedUInt32(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedUInt32(msg, field, i, temp);
                }
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;
                    auto now = ref->GetRepeatedUInt64(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    ref->SetRepeatedUInt64(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedUInt64(msg, field, i, temp);
                }
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;
                    auto now = ref->GetRepeatedDouble(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    ref->SetRepeatedDouble(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedDouble(msg, field, i, temp);
                }
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;
                    auto now = ref->GetRepeatedFloat(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    ref->SetRepeatedFloat(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedFloat(msg, field, i, temp);
                }
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;
                    auto now = ref->GetRepeatedBool(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    ref->SetRepeatedInt32(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedInt32(msg, field, i, temp);
                }
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                for(int i = 0; i < len; i++){
                    if(!CanMutate()) continue;
                    auto now = ref->GetRepeatedEnumValue(*msg, field, i);
                    auto temp = now;
                    mutate(&now);
                    NotNegMod(now, field->enum_type()->value_count());
                    ref->SetRepeatedEnumValue(msg, field, i, now);
                    if(max_size - GetMessageSize(msg) < 0) ref->SetRepeatedEnumValue(msg, field, i, temp);
                }              
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:
            case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        remain_size = max_size - GetMessageSize(msg);
    }

    void MutateSetField(Message* msg, const FieldDescriptor* field, int& remain_size){
        auto ref = msg->GetReflection();
        auto max_size = GetMessageSize(msg) + remain_size;
        // Special judgment for oneof fields
        if(auto oneof_desc = field->containing_oneof()){
            int index = field->index_in_oneof();
            int newIndex = index;
            mutate(&newIndex);
            NotNegMod(newIndex, oneof_desc->field_count());
            // If the index does not change, then mutate the field itself
            if(index == newIndex) return;
            auto new_field = oneof_desc->field(newIndex);
            if(IsMessageType(new_field)){
                auto new_msg = ref->GetMessage(*msg, new_field).New();
                // XXX: Unable to get the byte size of the field
                createRandomMessage(new_msg, remain_size);
                ref->SetAllocatedMessage(msg, new_msg, new_field);
                remain_size = max_size - GetMessageSize(msg);
                return;
            }
            else field = new_field; 
        }
        switch (field->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:{
                auto now = ref->GetInt32(*msg, field);
                auto temp = now;
                mutate(&now);
                ref->SetInt32(msg, field, now);
                if(max_size - GetMessageSize(msg) < 0) ref->SetInt32(msg, field, temp);
                break;
            } case FieldDescriptor::CPPTYPE_INT64:{
                auto now = ref->GetInt64(*msg, field);
                auto temp = now;
                mutate(&now);
                ref->SetInt64(msg, field, now);
                if(max_size - GetMessageSize(msg) < 0) ref->SetInt64(msg, field, temp);
                break;
            } case FieldDescriptor::CPPTYPE_UINT32:{
                auto now = ref->GetUInt32(*msg, field);
                auto temp = now;
                mutate(&now);
                ref->SetUInt32(msg, field, now);
                if(max_size - GetMessageSize(msg) < 0) ref->SetUInt32(msg, field, temp);
                break;
            } case FieldDescriptor::CPPTYPE_UINT64:{
                auto now = ref->GetUInt64(*msg, field);
                auto temp = now;
                mutate(&now);
                ref->SetUInt64(msg, field, now);
                if(max_size - GetMessageSize(msg) < 0) ref->SetUInt64(msg, field, temp);
                break;
            } case FieldDescriptor::CPPTYPE_DOUBLE:{
                auto now = ref->GetDouble(*msg, field);
                auto temp = now;
                mutate(&now);
                ref->SetDouble(msg, field, now);
                if(max_size - GetMessageSize(msg) < 0) ref->SetDouble(msg, field, temp);
                break;
            } case FieldDescriptor::CPPTYPE_FLOAT:{
                auto now = ref->GetFloat(*msg, field);
                auto temp = now;
                mutate(&now);
                ref->SetFloat(msg, field, now);
                if(max_size - GetMessageSize(msg) < 0) ref->SetFloat(msg, field, temp);
                break;
            } case FieldDescriptor::CPPTYPE_BOOL:{
                auto now = ref->GetBool(*msg, field);
                auto temp = now;
                mutate(&now);
                ref->SetBool(msg, field, now);
                if(max_size - GetMessageSize(msg) < 0) ref->SetBool(msg, field, temp);
                break;
            } case FieldDescriptor::CPPTYPE_ENUM:{
                auto now = ref->GetEnumValue(*msg, field);
                auto temp = now;
                mutate(&now);
                NotNegMod(now, field->enum_type()->value_count());
                ref->SetEnumValue(msg, field, now);     
                if(max_size - GetMessageSize(msg) < 0) ref->SetEnumValue(msg, field, temp);                   
                break;
            } 
            case FieldDescriptor::CPPTYPE_MESSAGE:
            case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        remain_size = max_size - GetMessageSize(msg);
    }

    void ShuffleRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size){
        auto ref = msg->GetReflection();
        auto max_size = GetMessageSize(msg) + remain_size;
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
        // Assume that shuffling the repeated field will not affect its size
        remain_size = max_size - GetMessageSize(msg);
    }
    
    void ReplaceRepeatedField(Message* msg1, const Message* msg2, 
              const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size){
        auto ref1 = msg1->GetReflection();
        auto ref2 = msg2->GetReflection();
        auto len1 = ref1->FieldSize(*msg1, field1);
        auto len2 = ref2->FieldSize(*msg2, field2);
        if(len1 == 0 || len2 == 0) return;
        auto max_size = msg1->ByteSizeLong() + remain_size;
        vector<int> idx1(len1);
        for(int i = 0; i < len1; i++) idx1[i] = i;
        shuffle(idx1.begin(), idx1.end(), getRandEngine()->randLongEngine);
        auto cnt = GetRandomIndex(min(len1 - 1, MAX_REPLACE_REPEATED_SIZE));
        for(int i = 0; i < cnt; i++){
            auto idx2 = GetRandomIndex(len2 - 1);
            switch (field1->cpp_type()){
                case FieldDescriptor::CPPTYPE_INT32:{
                    auto temp = ref1->GetRepeatedInt32(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedInt32(msg1, field1, idx1[i], ref2->GetRepeatedInt32(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedInt32(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_INT64:{
                    auto temp = ref1->GetRepeatedInt64(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedInt64(msg1, field1, idx1[i], ref2->GetRepeatedInt64(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedInt64(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_UINT32:{
                    auto temp = ref1->GetRepeatedUInt32(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedUInt32(msg1, field1, idx1[i], ref2->GetRepeatedUInt32(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedUInt32(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_UINT64:{
                    auto temp = ref1->GetRepeatedUInt64(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedUInt64(msg1, field1, idx1[i], ref2->GetRepeatedUInt64(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedUInt64(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_DOUBLE:{
                    auto temp = ref1->GetRepeatedDouble(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedDouble(msg1, field1, idx1[i], ref2->GetRepeatedDouble(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedDouble(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_FLOAT:{
                    auto temp = ref1->GetRepeatedFloat(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedFloat(msg1, field1, idx1[i], ref2->GetRepeatedFloat(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedFloat(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_BOOL:{
                    auto temp = ref1->GetRepeatedBool(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedBool(msg1, field1, idx1[i], ref2->GetRepeatedBool(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedBool(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_ENUM:{
                    auto temp = ref1->GetRepeatedEnumValue(*msg1, field1, idx1[i]);
                    ref1->SetRepeatedEnumValue(msg1, field1, idx1[i], ref2->GetRepeatedEnumValue(*msg2, field2, idx2));
                    if(max_size - msg1->ByteSizeLong() < 0)
                        ref1->SetRepeatedEnumValue(msg1, field1, idx1[i], temp);
                    break;
                }
                case FieldDescriptor::CPPTYPE_MESSAGE:{
                    // Do not replace if replacement would cause exceeding the maximum size
                    if(remain_size + ref1->GetRepeatedMessage(*msg1, field1, idx1[i]).ByteSizeLong() 
                                   - ref2->GetRepeatedMessage(*msg2, field2, idx2).ByteSizeLong() < 0)
                        break;
                    ref1->MutableRepeatedMessage(msg1, field1, idx1[i])->CopyFrom(ref2->GetRepeatedMessage(*msg2, field2, idx2));
                    break;
                }
                case FieldDescriptor::CPPTYPE_STRING:
                    // not support
                    break;
            }
            remain_size = max_size - msg1->ByteSizeLong();
        }
    }

    void ReplaceSetField(Message* msg1, const Message* msg2, 
         const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size){
        auto ref1 = msg1->GetReflection();
        auto ref2 = msg2->GetReflection();
        auto max_size = msg1->ByteSizeLong() + remain_size;
        Message* temp = msg1->New();
        temp->CopyFrom(*msg1);
        // Special judgment for oneof fields
        if(auto oneof_desc1 = field1->containing_oneof())
            field1 = oneof_desc1->field(field2->index_in_oneof());
        switch (field1->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:
                ref1->SetInt32(msg1, field1, ref2->GetInt32(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_INT64: 
                ref1->SetInt64(msg1, field1, ref2->GetInt64(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                ref1->SetUInt32(msg1, field1, ref2->GetUInt32(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                ref1->SetUInt64(msg1, field1, ref2->GetUInt64(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                ref1->SetDouble(msg1, field1, ref2->GetDouble(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                ref1->SetFloat(msg1, field1, ref2->GetFloat(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                ref1->SetBool(msg1, field1, ref2->GetBool(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                ref1->SetEnumValue(msg1, field1, ref2->GetEnumValue(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:
                ref1->MutableMessage(msg1, field1)->CopyFrom(ref2->GetMessage(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        // XXX: To avoid dealing with the complex situation of oneof field, 
        // just copy the whole original message to the crossover one in case of exceeding the maximum size
        remain_size = max_size - msg1->ByteSizeLong();
        if(remain_size < 0){
            msg1->CopyFrom(*temp);
            remain_size = max_size - msg1->ByteSizeLong();
        }
        delete temp;
    }

    void CrossoverAddRepeatedField(Message* msg1, const Message* msg2, 
                   const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size){
        auto ref1 = msg1->GetReflection();
        auto ref2 = msg2->GetReflection();
        auto len1 = ref1->FieldSize(*msg1, field1);
        auto len2 = ref2->FieldSize(*msg2, field2);
        auto max_size = msg1->ByteSizeLong() + remain_size;
        auto newLen = GetRandomIndex(min(len2, MAX_NEW_REPEATED_SIZE));
        vector<int> idx2(len2);
        for(int i = 0; i < len2; i++) idx2[i] = i;
        shuffle(idx2.begin(), idx2.end(), getRandEngine()->randLongEngine);
        for(int i = 0; i < newLen; i++){
            switch (field1->cpp_type()){
                case FieldDescriptor::CPPTYPE_INT32:
                    ref1->AddInt32(msg1, field1, ref2->GetRepeatedInt32(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_INT64:
                    ref1->AddInt64(msg1, field1, ref2->GetRepeatedInt64(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_UINT32:
                    ref1->AddUInt32(msg1, field1, ref2->GetRepeatedUInt32(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_UINT64:
                    ref1->AddUInt64(msg1, field1, ref2->GetRepeatedUInt64(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_DOUBLE:
                    ref1->AddDouble(msg1, field1, ref2->GetRepeatedDouble(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_FLOAT:
                    ref1->AddFloat(msg1, field1, ref2->GetRepeatedFloat(*msg2, field2, idx2[i]));
                    break;  
                case FieldDescriptor::CPPTYPE_BOOL:
                    ref1->AddBool(msg1, field1, ref2->GetRepeatedBool(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_ENUM:
                    ref1->AddEnumValue(msg1, field1, ref2->GetRepeatedEnumValue(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_MESSAGE:
                    ref1->AddMessage(msg1, field1)->CopyFrom(ref2->GetRepeatedMessage(*msg2, field2, idx2[i]));
                    break;
                case FieldDescriptor::CPPTYPE_STRING:  
                    // not support
                    break;
            }
            // check size
            remain_size = max_size - GetMessageSize(msg1);
            if(remain_size < 0) {
                ref1->RemoveLast(msg1, field1);
                remain_size = max_size - GetMessageSize(msg1);
                return;
            }      
        }
    }

    void CrossoverAddUnsetField(Message* msg1, const Message* msg2, 
                const FieldDescriptor* field1, const FieldDescriptor* field2, int& remain_size){
        auto ref1 = msg1->GetReflection();
        auto ref2 = msg2->GetReflection();
        auto max_size = msg1->ByteSizeLong() + remain_size;
        // Special judgment for oneof fields
        if(auto oneof_desc1 = field1->containing_oneof())
            field1 = oneof_desc1->field(field2->index_in_oneof());
        switch (field1->cpp_type()){
            case FieldDescriptor::CPPTYPE_INT32:
                ref1->SetInt32(msg1, field1, ref2->GetInt32(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_INT64: 
                ref1->SetInt64(msg1, field1, ref2->GetInt64(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                ref1->SetUInt32(msg1, field1, ref2->GetUInt32(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                ref1->SetUInt64(msg1, field1, ref2->GetUInt64(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                ref1->SetDouble(msg1, field1, ref2->GetDouble(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                ref1->SetFloat(msg1, field1, ref2->GetFloat(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                ref1->SetBool(msg1, field1, ref2->GetBool(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                ref1->SetEnumValue(msg1, field1, ref2->GetEnumValue(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:
                ref1->MutableMessage(msg1, field1)->CopyFrom(ref2->GetMessage(*msg2, field2));
                break;
            case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        // check size
        remain_size = max_size - GetMessageSize(msg1);
        if(remain_size < 0) {
            ref1->ClearField(msg1, field1);
            remain_size = max_size - GetMessageSize(msg1);
        }
    }
}