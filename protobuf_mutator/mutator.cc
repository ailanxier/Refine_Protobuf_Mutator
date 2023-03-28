#include "mutator.h"
namespace protobuf_mutator {

  using std::placeholders::_1;

namespace {

  const int kMaxInitializeDepth = 200;
  const uint64_t kDefaultMutateWeight = 1000000;

  enum class Mutation : uint8_t {
    None,
    Add,     // Adds new field with default value.
    Mutate,  // Mutates field contents.
    Delete,  // Deletes field.
    Copy,    // Copy values copied from another field.
    Clone,   // Create new field with value copied from another.
    Last = Clone,
  };

  using MutationBitset = std::bitset<static_cast<size_t>(Mutation::Last) + 1>;
  using Messages = std::vector<Message*>;
  using ConstMessages = std::vector<const Message*>;

  // Return random integer from [0, count)
  size_t GetRandomIndex(RandomEngine* random, size_t count) {
    assert(count > 0);
    if (count == 1) return 0;
    return std::uniform_int_distribution<size_t>(0, count - 1)(*random);
  }

  // Flips random bit in the buffer.
  void FlipBit(size_t size, uint8_t* bytes, RandomEngine* random) {
    size_t bit = GetRandomIndex(random, size * 8);
    bytes[bit / 8] ^= (1u << (bit % 8));
  }

  // Flips random bit in the value.
  template <class T>
  T FlipBit(T value, RandomEngine* random) {
    FlipBit(sizeof(value), reinterpret_cast<uint8_t*>(&value), random);
    return value;
  }

  // Return true with probability about 1-of-n.
  bool GetRandomBool(RandomEngine* random, size_t n = 2) { return GetRandomIndex(random, n) == 0;}

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
    bool ForType(const ConstFieldInstance& src, const ConstFieldInstance& dst, int size_increase_hint) const {
      T s;
      src.Load(&s);
      T d;
      dst.Load(&d);
      return SizeDiff(s, d) <= size_increase_hint && !IsEqual(s, d);
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

  // Selects random field and mutation from the given proto message.
  class MutationSampler {
  public:
    MutationSampler(bool keep_initialized, MutationBitset allowed_mutations, RandomEngine* random): 
    keep_initialized_(keep_initialized), allowed_mutations_(allowed_mutations), random_(random), sampler_(random) {}

    // Returns selected field.
    const FieldInstance& field() const { return sampler_.selected().field; }

    // Returns selected mutation.
    Mutation mutation() const { return sampler_.selected().mutation; }

    void Sample(Message* message) {
      SampleImpl(message);
      assert(mutation() != Mutation::None ||
            !allowed_mutations_[static_cast<size_t>(Mutation::Mutate)] ||
            message->GetDescriptor()->field_count() == 0);
    }

  private:
    bool keep_initialized_ = false;
    MutationBitset allowed_mutations_;
    RandomEngine* random_;
    struct Result {
      Result() = default;
      Result(const FieldInstance& f, Mutation m) : field(f), mutation(m) {}
      FieldInstance field;
      Mutation mutation = Mutation::None;
    };
    WeightedReservoirSampler<Result, RandomEngine> sampler_;

    void SampleImpl(Message* message) {
      const Descriptor* descriptor = message->GetDescriptor();
      const Reflection* reflection = message->GetReflection();

      int field_count = descriptor->field_count();
      for (int i = 0; i < field_count; ++i) {
        const FieldDescriptor* field = descriptor->field(i);
        if (const OneofDescriptor* oneof = field->containing_oneof()) {
          // Handle entire oneof group on the first field.
          if (field->index_in_oneof() == 0) {
            assert(oneof->field_count());
            const FieldDescriptor* current_field = reflection->GetOneofFieldDescriptor(*message, oneof);
            for (;;) {
              const FieldDescriptor* add_field = oneof->field(GetRandomIndex(random_, oneof->field_count()));
              if (add_field != current_field) {
                Try({message, add_field}, Mutation::Add);
                Try({message, add_field}, Mutation::Clone);
                break;
              }
              if (oneof->field_count() < 2) break;
            }
            if (current_field) {
              if (current_field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
                Try({message, current_field}, Mutation::Mutate);
              Try({message, current_field}, Mutation::Delete);
              Try({message, current_field}, Mutation::Copy);
            }
          }
        } else {
          if (field->is_repeated()) {
            int field_size = reflection->FieldSize(*message, field);
            size_t random_index = GetRandomIndex(random_, field_size + 1);
            Try({message, field, random_index}, Mutation::Add);
            Try({message, field, random_index}, Mutation::Clone);

            if (field_size) {
              size_t random_index = GetRandomIndex(random_, field_size);
              if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
                Try({message, field, random_index}, Mutation::Mutate);
              Try({message, field, random_index}, Mutation::Delete);
              Try({message, field, random_index}, Mutation::Copy);
            }
          } else {
            if (reflection->HasField(*message, field) || IsProto3SimpleField(*field)) {
              if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
                Try({message, field}, Mutation::Mutate);
              if (!IsProto3SimpleField(*field) && (!field->is_required() || !keep_initialized_)) {
                Try({message, field}, Mutation::Delete);
              }
              Try({message, field}, Mutation::Copy);
            } else {
              Try({message, field}, Mutation::Add);
              Try({message, field}, Mutation::Clone);
            }
          }
        }

        if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
          if (field->is_repeated()) {
            const int field_size = reflection->FieldSize(*message, field);
            for (int j = 0; j < field_size; ++j)
              SampleImpl(reflection->MutableRepeatedMessage(message, field, j));
          } else if (reflection->HasField(*message, field)) {
            SampleImpl(reflection->MutableMessage(message, field));
          }
        }
      }
    }
    void Try(const FieldInstance& field, Mutation mutation) {
      assert(mutation != Mutation::None);
      if (!allowed_mutations_[static_cast<size_t>(mutation)]) return;
      sampler_.Try(kDefaultMutateWeight, {field, mutation});
    }
  };

  // Selects random field of compatible type to use for clone mutations.
  class DataSourceSampler {
  public:
    DataSourceSampler(const ConstFieldInstance& match, RandomEngine* random, int size_increase_hint)
        : match_(match), random_(random), size_increase_hint_(size_increase_hint), sampler_(random) {}

    void Sample(const Message& message) { SampleImpl(message); }

    // Returns selected field.
    const ConstFieldInstance& field() const {
      assert(!IsEmpty());
      return sampler_.selected();
    }

    bool IsEmpty() const { return sampler_.IsEmpty(); }

  private:
    ConstFieldInstance match_;
    RandomEngine* random_;
    int size_increase_hint_;
    WeightedReservoirSampler<ConstFieldInstance, RandomEngine> sampler_;

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
            ConstFieldInstance source(&message, field, GetRandomIndex(random_, field_size));
            if (CanCopyAndDifferentField()(source, match_, size_increase_hint_))
              sampler_.Try(field_size, source);
          }
        } else {
          if (reflection->HasField(message, field)) {
            ConstFieldInstance source(&message, field);
            if (CanCopyAndDifferentField()(source, match_, size_increase_hint_))
              sampler_.Try(1, source);
          }
        }
      }
    }
  };

  }  // namespace

  class FieldMutator {
  public:
    FieldMutator(int size_increase_hint, bool enforce_changes, const ConstMessages& sources, Mutator* mutator)
    : size_increase_hint_(size_increase_hint), enforce_changes_(enforce_changes), sources_(sources), mutator_(mutator) {}

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

    void Mutate(std::unique_ptr<Message>* message) const {
      assert(!enforce_changes_);
      assert(*message);
      if (GetRandomBool(mutator_->random(), mutator_->random_to_default_ratio_)) return;
      mutator_->MutateImpl(sources_, {message->get()}, false, size_increase_hint_);
    }

  private:
    int size_increase_hint_;
    size_t enforce_changes_;
    const ConstMessages& sources_;
    Mutator* mutator_;
    
    template <class T, class F>
    void RepeatMutate(T* value, F mutate) const {
      if (!enforce_changes_ && GetRandomBool(mutator_->random(), mutator_->random_to_default_ratio_)) 
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
    void ForType(const FieldInstance& field, int size_increase_hint,
                const ConstMessages& sources, Mutator* mutator) const {
      T value;
      field.Load(&value);
      FieldMutator(size_increase_hint, true, sources, mutator).Mutate(&value);
      field.Store(value);
    }
  };

  struct CreateField : public FieldFunction<CreateField> {
  public:
    template <class T>
    void ForType(const FieldInstance& field, int size_increase_hint,
                const ConstMessages& sources, Mutator* mutator) const {
      T value;
      field.GetDefault(&value);
      /* defaults could be useful */
      FieldMutator field_mutator(size_increase_hint, false, sources, mutator);
      field_mutator.Mutate(&value);
      field.Create(value);
    }
  };

  }  // namespace

  void Mutator::Seed(uint32_t value) { random_.seed(value); }

  void Mutator::Mutate(Message* message, size_t max_size_hint) { 
    Messages messages;
    messages.push_back(message);

    ConstMessages sources(messages.begin(), messages.end());
    MutateImpl(sources, messages, false, static_cast<int>(max_size_hint) - static_cast<int>(message->ByteSizeLong()));
  }

  void Mutator::CrossOver(const Message& message1, Message* message2, size_t max_size_hint) {
    Messages messages;
    messages.push_back(message2);

    ConstMessages sources;
    sources.push_back(&message1);
    sources.push_back(message2);

    MutateImpl(sources, messages, true, static_cast<int>(max_size_hint) - static_cast<int>(message2->ByteSizeLong()));
  }

  bool Mutator::MutateImpl(const ConstMessages& sources, const Messages& messages,
                          bool copy_clone_only, int size_increase_hint) {
    MutationBitset mutations;
    if (copy_clone_only) {
      mutations[static_cast<size_t>(Mutation::Copy)] = true;
      mutations[static_cast<size_t>(Mutation::Clone)] = true;
    } else if (size_increase_hint <= 16) {
      mutations[static_cast<size_t>(Mutation::Delete)] = true;
    } else {
      mutations.set();
      mutations[static_cast<size_t>(Mutation::Copy)] = false;
      mutations[static_cast<size_t>(Mutation::Clone)] = false;
    }
    while (mutations.any()) {
      MutationSampler mutation(keep_initialized_, mutations, &random_);
      for (Message* message : messages) mutation.Sample(message);

      switch (mutation.mutation()) {
        case Mutation::None:
          return true;
        case Mutation::Add:
          CreateField()(mutation.field(), size_increase_hint, sources, this);
          return true;
        case Mutation::Mutate:
          MutateField()(mutation.field(), size_increase_hint, sources, this);
          return true;
        case Mutation::Delete:
          DeleteField()(mutation.field());
          return true;
        case Mutation::Clone: {
          CreateDefaultField()(mutation.field());
          DataSourceSampler source_sampler(mutation.field(), &random_, size_increase_hint);
          for (const Message* source : sources) source_sampler.Sample(*source);
          if (source_sampler.IsEmpty()) {
            if (!IsProto3SimpleField(*mutation.field().descriptor()))
              return true;  // CreateField is enough for proto2.
            break;
          }
          CopyField()(source_sampler.field(), mutation.field());
          return true;
        }
        case Mutation::Copy: {
          DataSourceSampler source_sampler(mutation.field(), &random_, size_increase_hint);
          for (const Message* source : sources) source_sampler.Sample(*source);
          if (source_sampler.IsEmpty()) break;
          CopyField()(source_sampler.field(), mutation.field());
          return true;
        }
        default:
          assert(false && "unexpected mutation");
          return false;
      }

      // Don't try same mutation next time.
      mutations[static_cast<size_t>(mutation.mutation())] = false;
    }
    return false;
  }

  int32_t Mutator::MutateInt32(int32_t value) { return rand(); }

  int64_t Mutator::MutateInt64(int64_t value) { return FlipBit(value, &random_); }

  uint32_t Mutator::MutateUInt32(uint32_t value) { return FlipBit(value, &random_);}

  uint64_t Mutator::MutateUInt64(uint64_t value) { return FlipBit(value, &random_); }

  float Mutator::MutateFloat(float value) { return FlipBit(value, &random_); }

  double Mutator::MutateDouble(double value) { return FlipBit(value, &random_); }

  bool Mutator::MutateBool(bool value) { return !value; }

  size_t Mutator::MutateEnum(size_t index, size_t item_count) {
    if (item_count <= 1) return 0;
    return (index + 1 + GetRandomIndex(&random_, item_count - 1)) % item_count;
  }
}  // namespace protobuf_mutator
