#include "mutate_util.h"

namespace protobuf_mutator {
  
    void createRandomMessage(Message* msg, int& remain_size, Random* rand){
        auto desc = msg->GetDescriptor();
        auto ref = msg->GetReflection();
        auto field_count = desc->field_count();
        for (int i = 0; i < field_count; i++){
            auto field = desc->field(i);
            if (field->is_repeated())
                AddRepeatedField(msg, field, remain_size, rand);
            else if(auto oneof_desc = field->containing_oneof()){
                auto new_desc = oneof_desc->field(rand->randomIndex(oneof_desc->field_count() - 1));
                i += oneof_desc->field_count() - 1;
                if(new_desc->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE){
                    auto new_msg = ref->MutableMessage(msg, new_desc)->New();
                    remain_size -= new_msg->ByteSizeLong();
                    createRandomMessage(new_msg, remain_size, rand);
                    ref->SetAllocatedMessage(msg, new_msg, new_desc);
                }else
                    AddUnsetField(msg, new_desc, remain_size, rand);
            }else
                AddUnsetField(msg, field, remain_size, rand);
        }
    }

    void AddRepeatedField(Message* msg, const FieldDescriptor* field, int& remain_size, Random* rand){
        auto ref = msg->GetReflection();
        auto type = field->cpp_type();
        int oldLen = ref->FieldSize(*msg, field);
        // Temporarily proposed to add a length no more than MAX_NEW_REPEATED_SIZE
        int newLen = oldLen + rand->randomIndex(MAX_NEW_REPEATED_SIZE);
        for(int i = oldLen + 1; i <= newLen; i++){
            auto oldSize = msg->ByteSizeLong();
            // add random field
            switch (type){
                case FieldDescriptor::CPPTYPE_INT32:
                    ref->AddInt32(msg, field, rand->randomNum(INT32_MIN, INT32_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_INT64:
                    ref->AddInt64(msg, field, rand->randomNum(INT64_MIN, INT64_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_UINT32:
                    ref->AddUInt32(msg, field, rand->randomIndex(UINT32_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_UINT64:
                    ref->AddUInt64(msg, field, rand->randomIndex(UINT64_MAX));
                    break;
                case FieldDescriptor::CPPTYPE_DOUBLE:
                    // small real number
                    ref->AddDouble(msg, field, rand->randomNum(-1.0, 1.0));
                    break;
                case FieldDescriptor::CPPTYPE_FLOAT:
                    ref->AddFloat(msg, field, rand->randomNum(-1.0, 1.0));
                    break;
                case FieldDescriptor::CPPTYPE_BOOL:
                    ref->AddBool(msg, field, rand->randomIndex(1));
                    break;
                case FieldDescriptor::CPPTYPE_ENUM:
                    ref->AddEnum(msg, field, field->enum_type()->value(rand->randomIndex(field->enum_type()->value_count() - 1)));
                    break;
                case FieldDescriptor::CPPTYPE_MESSAGE:{
                    auto newMsg = ref->AddMessage(msg, field);
                    int msg_max_size = remain_size - (msg->ByteSizeLong() - oldSize);
                    createRandomMessage(newMsg, msg_max_size, rand);
                    break;
                }
                case FieldDescriptor::CPPTYPE_STRING:
                    // not support
                    break;
            }
            // check size
            auto newSize = msg->ByteSizeLong();
            remain_size -= (newSize - oldSize);
            if(remain_size < 0) {
                ref->RemoveLast(msg, field);
                remain_size += (newSize - oldSize);
                return;
            }      
        }
    }

    void AddUnsetField(Message* msg, const FieldDescriptor* field, int& remain_size, Random* rand){
        auto ref = msg->GetReflection();
        auto type = field->cpp_type();
        auto oldSize = msg->ByteSizeLong();
        switch (type){
            case FieldDescriptor::CPPTYPE_INT32:
                ref->SetInt32(msg, field, rand->randomNum(INT32_MIN, INT32_MAX));
                break;
            case FieldDescriptor::CPPTYPE_INT64:
                ref->SetInt64(msg, field, rand->randomNum(INT64_MIN, INT64_MAX));
                break;
            case FieldDescriptor::CPPTYPE_UINT32:
                ref->SetUInt32(msg, field, rand->randomIndex(UINT32_MAX));
                break;
            case FieldDescriptor::CPPTYPE_UINT64:
                ref->SetUInt64(msg, field, rand->randomIndex(UINT64_MAX));
                break;
            case FieldDescriptor::CPPTYPE_DOUBLE:
                // small real number
                ref->SetDouble(msg, field, rand->randomNum(-1.0, 1.0));
                break;
            case FieldDescriptor::CPPTYPE_FLOAT:
                ref->SetFloat(msg, field, rand->randomNum(-1.0, 1.0));
                break;
            case FieldDescriptor::CPPTYPE_BOOL:
                ref->SetBool(msg, field, rand->randomIndex(1));
                break;
            case FieldDescriptor::CPPTYPE_ENUM:
                ref->SetEnum(msg, field, field->enum_type()->value(rand->randomIndex(field->enum_type()->value_count() - 1)));
                break;
            case FieldDescriptor::CPPTYPE_MESSAGE:{
                auto newMsg = ref->MutableMessage(msg, field);
                int msg_max_size = remain_size - (msg->ByteSizeLong() - oldSize);
                createRandomMessage(newMsg, msg_max_size, rand);
                break;
            }case FieldDescriptor::CPPTYPE_STRING:
                // not support
                break;
        }
        // check size
        auto newSize = msg->ByteSizeLong();
        remain_size -= (newSize - oldSize);
        if(remain_size < 0) {
        ref->ClearField(msg, field);
        remain_size += (newSize - oldSize);
        }
    }

    void DeleteRepeatedField(Message* msg, const FieldDescriptor* field, Random* rand){

    }
    void DeleteSetField(Message* msg, const FieldDescriptor* field, Random* rand){
        
    }
}