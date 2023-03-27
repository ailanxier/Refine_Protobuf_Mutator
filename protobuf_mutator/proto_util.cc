#include "proto_util.h"
#include "mutator.h"

namespace protobuf_mutator {

namespace {
  class InputReader {
  public:
    InputReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}
    virtual ~InputReader() = default;

    virtual bool Read(Message* message) const = 0;

    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }

  private:
    const uint8_t* data_;
    size_t size_;
  };

  class OutputWriter {
  public:
    OutputWriter(uint8_t* data, size_t size) : data_(data), size_(size) {}
    virtual ~OutputWriter() = default;

    virtual size_t Write(const Message& message) = 0;

    uint8_t* data() const { return data_; }
    size_t size() const { return size_; }

  private:
    uint8_t* data_;
    size_t size_;
  };

  class TextInputReader : public InputReader {
  public:
    using InputReader::InputReader;

    bool Read(Message* message) const override {
      return ParseTextMessage(data(), size(), message);
    }
  };

  class TextOutputWriter : public OutputWriter {
  public:
    using OutputWriter::OutputWriter;

    size_t Write(const Message& message) override {
      return SaveMessageAsText(message, data(), size());
    }
  };

  class BinaryInputReader : public InputReader {
  public:
    using InputReader::InputReader;

    bool Read(Message* message) const override {
      return ParseBinaryMessage(data(), size(), message);
    }
  };

  class BinaryOutputWriter : public OutputWriter {
  public:
    using OutputWriter::OutputWriter;

    size_t Write(const Message& message) override {
      return SaveMessageAsBinary(message, data(), size());
    }
  };

  class LastMutationCache {
    public:
      void Store(const uint8_t* data, size_t size, Message* message) {
        if (!message_) message_.reset(message->New());
        message->GetReflection()->Swap(message, message_.get());
        data_.assign(data, data + size);
      }

      bool LoadIfSame(const uint8_t* data, size_t size,
                      Message* message) {
        if (!message_ || size != data_.size() ||
            !std::equal(data_.begin(), data_.end(), data))
          return false;

        message->GetReflection()->Swap(message, message_.get());
        message_.reset();
        return true;
      }

    private:
      std::vector<uint8_t> data_;
      std::unique_ptr<Message> message_;
  };

  LastMutationCache* GetCache() {
    static LastMutationCache cache;
    return &cache;
  }

  Mutator* GetMutator() {
    static Mutator mutator;
    return &mutator;
  }

  size_t GetMaxSize(const InputReader& input, const OutputWriter& output,
                    const Message& message) {
    size_t max_size = message.ByteSizeLong() + output.size();
    max_size -= std::min(max_size, input.size());
    return max_size;
    return output.size();
  }

  size_t MutateMessage(unsigned int seed, const InputReader& input, OutputWriter* output, Message* message) {
    GetMutator()->Seed(seed);
    input.Read(message);
    size_t max_size = GetMaxSize(input, *output, *message);
    GetMutator()->Mutate(message, max_size);
    if (size_t new_size = output->Write(*message)) {
      assert(new_size <= output->size());
      GetCache()->Store(output->data(), new_size, message);
      return new_size;
    }
    return 0;
  }

  size_t CrossOverMessages(unsigned int seed, const InputReader& input1, const InputReader& input2, 
                           OutputWriter* output, Message* message1, Message* message2) {
    GetMutator()->Seed(seed);
    input1.Read(message1);
    input2.Read(message2);
    size_t max_size = GetMaxSize(input1, *output, *message1);
    GetMutator()->CrossOver(*message2, message1, max_size);
    if (size_t new_size = output->Write(*message1)) {
      assert(new_size <= output->size());
      GetCache()->Store(output->data(), new_size, message1);
      return new_size;
    }
    return 0;
  }

  size_t MutateTextMessage(uint8_t* data, size_t size, size_t max_size, unsigned int seed, Message* message) {
    TextInputReader input(data, size);
    TextOutputWriter output(data, max_size);
    return MutateMessage(seed, input, &output, message);
  }

  size_t CrossOverTextMessages(const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2, 
                uint8_t* out, size_t max_out_size, unsigned int seed, Message* message1, Message* message2) {
    TextInputReader input1(data1, size1);
    TextInputReader input2(data2, size2);
    TextOutputWriter output(out, max_out_size);
    return CrossOverMessages(seed, input1, input2, &output, message1, message2);
  }

  size_t MutateBinaryMessage(uint8_t* data, size_t size, size_t max_size, unsigned int seed, Message* message) {
    BinaryInputReader input(data, size);
    BinaryOutputWriter output(data, max_size);
    return MutateMessage(seed, input, &output, message);
  }

  size_t CrossOverBinaryMessages(const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2, 
                uint8_t* out, size_t max_out_size, unsigned int seed, Message* message1, Message* message2) {
    BinaryInputReader input1(data1, size1);
    BinaryInputReader input2(data2, size2);
    BinaryOutputWriter output(out, max_out_size);
    return CrossOverMessages(seed, input1, input2, &output, message1, message2);
  }

}  // namespace

  size_t CustomProtoMutator(bool binary, uint8_t* data, size_t size, size_t max_size, unsigned int seed,
                            Message* input) {
    auto mutate = binary ? &MutateBinaryMessage : &MutateTextMessage;
    return mutate(data, size, max_size, seed, input);
  }

  size_t CustomProtoCrossOver(bool binary, const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2, 
                      uint8_t* out, size_t max_out_size, unsigned int seed, Message* input1, Message* input2) {
    auto cross = binary ? &CrossOverBinaryMessages : &CrossOverTextMessages;
    return cross(data1, size1, data2, size2, out, max_out_size, seed, input1, input2);
  }

  bool LoadProtoInput(bool binary, const uint8_t* data, size_t size, Message* input) {
    if (GetCache()->LoadIfSame(data, size, input)) return true;
    auto result = binary ? ParseBinaryMessage(data, size, input) : ParseTextMessage(data, size, input);
    if (!result) return false;
    GetMutator()->Seed(size);
    GetMutator()->Fix(input);
    return true;
  }

  void RegisterPostProcessor(const Descriptor* desc,
      std::function<void(Message* message, unsigned int seed)> callback) {
    GetMutator()->RegisterPostProcessor(desc, callback);
  }

bool ParseBinaryMessage(const String& data, Message* output) {
    output->Clear();
    if (!output->ParsePartialFromString(data)) {
      output->Clear();
      return false;
    }
    return true;
  }

  bool ParseBinaryMessage(const uint8_t* data, size_t size, Message* output) {
    return ParseBinaryMessage({reinterpret_cast<const char*>(data), size}, output);
  }

  String SaveMessageAsBinary(const Message& message) {
    String tmp;
    if (!message.SerializePartialToString(&tmp)) return {};
    return tmp;
  }

  size_t SaveMessageAsBinary(const Message& message, uint8_t* data, size_t max_size) {
    String result = SaveMessageAsBinary(message);
    if (result.size() <= max_size) {
      memcpy(data, result.data(), result.size());
      return result.size();
    }
    return 0;
  }

  bool ParseTextMessage(const String& data, Message* output) {
    output->Clear();
    TextFormat::Parser parser;
    parser.SetRecursionLimit(100);
    parser.AllowPartialMessage(true);
    parser.AllowUnknownField(true);
    if (!parser.ParseFromString(data, output)) {
      output->Clear();
      return false;
    }
    return true;
  }

  bool ParseTextMessage(const uint8_t* data, size_t size, Message* output) {
    return ParseTextMessage({reinterpret_cast<const char*>(data), size}, output);
  }

  String SaveMessageAsText(const Message& message) {
    String tmp;
    if (!TextFormat::PrintToString(message, &tmp)) return {};
    return tmp;
  }

  size_t SaveMessageAsText(const Message& message, uint8_t* data, size_t max_size) {
    String result = SaveMessageAsText(message);
    if (result.size() <= max_size) {
      memcpy(data, result.data(), result.size());
      return result.size();
    }
    return 0;
  }

  namespace {
    void StoreCode(char* e, char32_t code, uint8_t size, uint8_t prefix) {
      while (--size) {
        *(--e) = 0x80 | (code & 0x3F);
        code >>= 6;
      }
      *(--e) = prefix | code;
    }

    char* FixCode(char* b, const char* e, RandomEngine* random) {
      const char* start = b;
      assert(b < e);

      e = std::min<const char*>(e, b + 4);
      char32_t c = *b++;
      for (; b < e && (*b & 0xC0) == 0x80; ++b)
        c = (c << 6) + (*b & 0x3F);
      uint8_t size = b - start;
      switch (size) {
        case 1:
          c &= 0x7F;
          StoreCode(b, c, size, 0);
          break;
        case 2:
          c &= 0x7FF;
          if (c < 0x80) {
            // Use uint32_t because uniform_int_distribution does not support char32_t on Windows.
            c = std::uniform_int_distribution<uint32_t>(0x80, 0x7FF)(*random);
          }
          StoreCode(b, c, size, 0xC0);
          break;
        case 3:
          c &= 0xFFFF;

          // [0xD800, 0xE000) are reserved for UTF-16 surrogate halves.
          if (c < 0x800 || (c >= 0xD800 && c < 0xE000)) {
            uint32_t halves = 0xE000 - 0xD800;
            c = std::uniform_int_distribution<uint32_t>(0x800, 0xFFFF - halves)(*random);
            if (c >= 0xD800) c += halves;
          }
          StoreCode(b, c, size, 0xE0);
          break;
        case 4:
          c &= 0x1FFFFF;
          if (c < 0x10000 || c > 0x10FFFF) 
            c = std::uniform_int_distribution<uint32_t>(0x10000, 0x10FFFF)(*random);
          StoreCode(b, c, size, 0xF0);
          break;
        default:
          assert(false && "Unexpected size of UTF-8 sequence");
      }
      return b;
    }

  }  // namespace

  void FixUtf8String(String* str, RandomEngine* random) {
    if (str->empty()) return;
    char* b = &(*str)[0];
    const char* e = b + str->size();
    while (b < e) {
      b = FixCode(b, e, random);
    }
  }
}  // namespace protobuf_mutator
