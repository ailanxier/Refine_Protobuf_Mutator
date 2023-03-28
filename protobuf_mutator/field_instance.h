#ifndef SRC_FIELD_INSTANCE_H_
#define SRC_FIELD_INSTANCE_H_

#include <memory>
#include <string>

namespace protobuf_mutator {
  using protobuf::Message;
  using protobuf::Descriptor;
  using protobuf::FieldDescriptor;
  using protobuf::FileDescriptor;
  using protobuf::OneofDescriptor;
  using protobuf::Reflection;
  using protobuf::EnumDescriptor;
  using protobuf::EnumValueDescriptor;

// Helper class for common protobuf fields operations.
class ConstFieldInstance {
 public:
  static const size_t kInvalidIndex = -1;

  struct Enum {
    size_t index;
    size_t count;
  };

  ConstFieldInstance(): message_(nullptr), descriptor_(nullptr), index_(kInvalidIndex) {}

  ConstFieldInstance(const Message* message, const FieldDescriptor* field, size_t index)
      : message_(message), descriptor_(field), index_(index) {
    assert(message_);
    assert(descriptor_);
    assert(index_ != kInvalidIndex);
    assert(descriptor_->is_repeated());
  }

  ConstFieldInstance(const Message* message, const FieldDescriptor* field)
      : message_(message), descriptor_(field), index_(kInvalidIndex) {
    assert(message_);
    assert(descriptor_);
    assert(!descriptor_->is_repeated());
  }

  void GetDefault(int32_t* out) const {*out = descriptor_->default_value_int32();}

  void GetDefault(int64_t* out) const {*out = descriptor_->default_value_int64();}

  void GetDefault(uint32_t* out) const {*out = descriptor_->default_value_uint32();}

  void GetDefault(uint64_t* out) const {*out = descriptor_->default_value_uint64();}

  void GetDefault(double* out) const {*out = descriptor_->default_value_double();}

  void GetDefault(float* out) const {*out = descriptor_->default_value_float();}

  void GetDefault(bool* out) const { *out = descriptor_->default_value_bool(); }

  void GetDefault(Enum* out) const {
    const EnumValueDescriptor* value = descriptor_->default_value_enum();
    const EnumDescriptor* type = value->type();
    *out = {static_cast<size_t>(value->index()), static_cast<size_t>(type->value_count())};
  }

  void GetDefault(std::unique_ptr<Message>* out) const {
    out->reset(reflection().GetMessageFactory()->GetPrototype(descriptor_->message_type())->New());
  }

  void Load(int32_t* value) const {
    *value = is_repeated()? reflection().GetRepeatedInt32(*message_, descriptor_, index_)
                          : reflection().GetInt32(*message_, descriptor_);
  }

  void Load(int64_t* value) const {
    *value = is_repeated()
                 ? reflection().GetRepeatedInt64(*message_, descriptor_, index_)
                 : reflection().GetInt64(*message_, descriptor_);
  }

  void Load(uint32_t* value) const {
    *value = is_repeated() ? reflection().GetRepeatedUInt32(*message_, descriptor_, index_)
                           : reflection().GetUInt32(*message_, descriptor_);
  }

  void Load(uint64_t* value) const {
    *value = is_repeated() ? reflection().GetRepeatedUInt64(*message_, descriptor_, index_)
                           : reflection().GetUInt64(*message_, descriptor_);
  }

  void Load(double* value) const {
    *value = is_repeated() ? reflection().GetRepeatedDouble(*message_, descriptor_, index_)
                           : reflection().GetDouble(*message_, descriptor_);
  }

  void Load(float* value) const {
    *value = is_repeated()? reflection().GetRepeatedFloat(*message_, descriptor_, index_)
                          : reflection().GetFloat(*message_, descriptor_);
  }

  void Load(bool* value) const {
    *value = is_repeated()? reflection().GetRepeatedBool(*message_, descriptor_, index_)
                          : reflection().GetBool(*message_, descriptor_);
  }

  void Load(Enum* value) const {
    const EnumValueDescriptor* value_descriptor = is_repeated()
            ? reflection().GetRepeatedEnum(*message_, descriptor_, index_)
            : reflection().GetEnum(*message_, descriptor_);
    *value = {static_cast<size_t>(value_descriptor->index()),
              static_cast<size_t>(value_descriptor->type()->value_count())};
    if (value->index >= value->count) GetDefault(value);
  }

  void Load(std::unique_ptr<Message>* value) const {
    const Message& source = is_repeated()
            ? reflection().GetRepeatedMessage(*message_, descriptor_, index_)
            : reflection().GetMessage(*message_, descriptor_);
    value->reset(source.New());
    (*value)->CopyFrom(source);
  }

  std::string name() const { return descriptor_->name(); }

  FieldDescriptor::CppType cpp_type() const {return descriptor_->cpp_type();}

  const EnumDescriptor* enum_type() const {return descriptor_->enum_type();}

  const Descriptor* message_type() const {return descriptor_->message_type();}

  const FieldDescriptor* descriptor() const { return descriptor_; }

  std::string DebugString() const {
    std::string s = descriptor_->DebugString();
    if (is_repeated()) s += "[" + std::to_string(index_) + "]";
    return s + " of\n" + message_->DebugString();
  }

 protected:
  bool is_repeated() const { return descriptor_->is_repeated(); }

  const Reflection& reflection() const { return *message_->GetReflection(); }

  size_t index() const { return index_; }

 private:
  template <class Fn, class T>
  friend struct FieldFunction;

  const Message* message_;
  const FieldDescriptor* descriptor_;
  size_t index_;
};

class FieldInstance : public ConstFieldInstance {
 public:
  static const size_t kInvalidIndex = -1;

  FieldInstance() : ConstFieldInstance(), message_(nullptr) {}

  FieldInstance(Message* message, const FieldDescriptor* field, size_t index)
      : ConstFieldInstance(message, field, index), message_(message) {}

  FieldInstance(Message* message, const FieldDescriptor* field)
      : ConstFieldInstance(message, field), message_(message) {}

  void Delete() const {
    if (!is_repeated()) return reflection().ClearField(message_, descriptor());
    int field_size = reflection().FieldSize(*message_, descriptor());
    // API has only method to delete the last message, so we move method from the middle to the end.
    for (int i = index() + 1; i < field_size; ++i)
      reflection().SwapElements(message_, descriptor(), i, i - 1);
    reflection().RemoveLast(message_, descriptor());
  }

  template <class T>
  void Create(const T& value) const {
    if (!is_repeated()) return Store(value);
    InsertRepeated(value);
  }

  void Store(int32_t value) const {
    if (is_repeated())
      reflection().SetRepeatedInt32(message_, descriptor(), index(), value);
    else
      reflection().SetInt32(message_, descriptor(), value);
  }

  void Store(int64_t value) const {
    if (is_repeated())
      reflection().SetRepeatedInt64(message_, descriptor(), index(), value);
    else
      reflection().SetInt64(message_, descriptor(), value);
  }

  void Store(uint32_t value) const {
    if (is_repeated())
      reflection().SetRepeatedUInt32(message_, descriptor(), index(), value);
    else
      reflection().SetUInt32(message_, descriptor(), value);
  }

  void Store(uint64_t value) const {
    if (is_repeated())
      reflection().SetRepeatedUInt64(message_, descriptor(), index(), value);
    else
      reflection().SetUInt64(message_, descriptor(), value);
  }

  void Store(double value) const {
    if (is_repeated())
      reflection().SetRepeatedDouble(message_, descriptor(), index(), value);
    else
      reflection().SetDouble(message_, descriptor(), value);
  }

  void Store(float value) const {
    if (is_repeated())
      reflection().SetRepeatedFloat(message_, descriptor(), index(), value);
    else
      reflection().SetFloat(message_, descriptor(), value);
  }

  void Store(bool value) const {
    if (is_repeated())
      reflection().SetRepeatedBool(message_, descriptor(), index(), value);
    else
      reflection().SetBool(message_, descriptor(), value);
  }

  void Store(const Enum& value) const {
    assert(value.index < value.count);
    const EnumValueDescriptor* enum_value =
        descriptor()->enum_type()->value(value.index);
    if (is_repeated())
      reflection().SetRepeatedEnum(message_, descriptor(), index(), enum_value);
    else
      reflection().SetEnum(message_, descriptor(), enum_value);
  }

  void Store(const std::unique_ptr<Message>& value) const {
    Message* mutable_message = is_repeated() 
                      ? reflection().MutableRepeatedMessage(message_, descriptor(), index())
                      : reflection().MutableMessage(message_, descriptor());
    mutable_message->Clear();
    if (value) mutable_message->CopyFrom(*value);
  }

 private:
  Message* message_;

  template <class T>
  void InsertRepeated(const T& value) const {
    PushBackRepeated(value);
    size_t field_size = reflection().FieldSize(*message_, descriptor());
    if (field_size == 1) return;
    // API has only method to add field to the end of the list. So we add descriptor()
    // and move it into the middle.
    for (size_t i = field_size - 1; i > index(); --i)
      reflection().SwapElements(message_, descriptor(), i, i - 1);
  }

  void PushBackRepeated(int32_t value) const {
    assert(is_repeated());
    reflection().AddInt32(message_, descriptor(), value);
  }

  void PushBackRepeated(int64_t value) const {
    assert(is_repeated());
    reflection().AddInt64(message_, descriptor(), value);
  }

  void PushBackRepeated(uint32_t value) const {
    assert(is_repeated());
    reflection().AddUInt32(message_, descriptor(), value);
  }

  void PushBackRepeated(uint64_t value) const {
    assert(is_repeated());
    reflection().AddUInt64(message_, descriptor(), value);
  }

  void PushBackRepeated(double value) const {
    assert(is_repeated());
    reflection().AddDouble(message_, descriptor(), value);
  }

  void PushBackRepeated(float value) const {
    assert(is_repeated());
    reflection().AddFloat(message_, descriptor(), value);
  }

  void PushBackRepeated(bool value) const {
    assert(is_repeated());
    reflection().AddBool(message_, descriptor(), value);
  }

  void PushBackRepeated(const Enum& value) const {
    assert(value.index < value.count);
    const EnumValueDescriptor* enum_value = descriptor()->enum_type()->value(value.index);
    assert(is_repeated());
    reflection().AddEnum(message_, descriptor(), enum_value);
  }

  void PushBackRepeated(const std::unique_ptr<Message>& value) const {
    assert(is_repeated());
    Message* mutable_message = reflection().AddMessage(message_, descriptor());
    mutable_message->Clear();
    if (value) mutable_message->CopyFrom(*value);
  }

};

template <class Fn, class R = void>
struct FieldFunction {
  template <class Field, class... Args>
  R operator()(const Field& field, const Args&... args) const {
    assert(field.descriptor());
    switch (field.cpp_type()) {
      case FieldDescriptor::CPPTYPE_INT32:
        return static_cast<const Fn*>(this)->template ForType<int32_t>(field, args...);
      case FieldDescriptor::CPPTYPE_INT64:
        return static_cast<const Fn*>(this)->template ForType<int64_t>(field, args...);
      case FieldDescriptor::CPPTYPE_UINT32:
        return static_cast<const Fn*>(this)->template ForType<uint32_t>(field, args...);
      case FieldDescriptor::CPPTYPE_UINT64:
        return static_cast<const Fn*>(this)->template ForType<uint64_t>(field, args...);
      case FieldDescriptor::CPPTYPE_DOUBLE:
        return static_cast<const Fn*>(this)->template ForType<double>(field, args...);
      case FieldDescriptor::CPPTYPE_FLOAT:
        return static_cast<const Fn*>(this)->template ForType<float>(field, args...);
      case FieldDescriptor::CPPTYPE_BOOL:
        return static_cast<const Fn*>(this)->template ForType<bool>(field, args...);
      case FieldDescriptor::CPPTYPE_ENUM:
        return static_cast<const Fn*>(this)->template ForType<ConstFieldInstance::Enum>(field, args...);
      // do not support string
      case FieldDescriptor::CPPTYPE_STRING:  
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE:
        return static_cast<const Fn*>(this)->template ForType<std::unique_ptr<Message>>(field, args...);
    }
    assert(false && "Unknown type");
    abort();
  }
};

}  // namespace protobuf_mutator

#endif  // SRC_FIELD_INSTANCE_H_
