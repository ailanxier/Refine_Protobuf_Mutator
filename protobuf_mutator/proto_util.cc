#include "proto_util.h"
#include "binary_format.h"
#include "text_format.h"
#include "mutator.h"

#include <algorithm>
#include <memory>
#include <vector>

namespace protobuf_mutator {

namespace {

class InputReader {
 public:
  InputReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}
  virtual ~InputReader() = default;

  virtual bool Read(protobuf::Message* message) const = 0;

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

  virtual size_t Write(const protobuf::Message& message) = 0;

  uint8_t* data() const { return data_; }
  size_t size() const { return size_; }

 private:
  uint8_t* data_;
  size_t size_;
};

class TextInputReader : public InputReader {
 public:
  using InputReader::InputReader;

  bool Read(protobuf::Message* message) const override {
    return ParseTextMessage(data(), size(), message);
  }
};

class TextOutputWriter : public OutputWriter {
 public:
  using OutputWriter::OutputWriter;

  size_t Write(const protobuf::Message& message) override {
    return SaveMessageAsText(message, data(), size());
  }
};

class BinaryInputReader : public InputReader {
 public:
  using InputReader::InputReader;

  bool Read(protobuf::Message* message) const override {
    return ParseBinaryMessage(data(), size(), message);
  }
};

class BinaryOutputWriter : public OutputWriter {
 public:
  using OutputWriter::OutputWriter;

  size_t Write(const protobuf::Message& message) override {
    return SaveMessageAsBinary(message, data(), size());
  }
};

class LastMutationCache {
 public:
  void Store(const uint8_t* data, size_t size, protobuf::Message* message) {
    if (!message_) message_.reset(message->New());
    message->GetReflection()->Swap(message, message_.get());
    data_.assign(data, data + size);
  }

  bool LoadIfSame(const uint8_t* data, size_t size,
                  protobuf::Message* message) {
    if (!message_ || size != data_.size() ||
        !std::equal(data_.begin(), data_.end(), data))
      return false;

    message->GetReflection()->Swap(message, message_.get());
    message_.reset();
    return true;
  }

 private:
  std::vector<uint8_t> data_;
  std::unique_ptr<protobuf::Message> message_;
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
                  const protobuf::Message& message) {
  size_t max_size = message.ByteSizeLong() + output.size();
  max_size -= std::min(max_size, input.size());
  return max_size;
  return output.size();
}

size_t MutateMessage(unsigned int seed, const InputReader& input,
                     OutputWriter* output, protobuf::Message* message) {
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

size_t CrossOverMessages(unsigned int seed, const InputReader& input1,
                         const InputReader& input2, OutputWriter* output,
                         protobuf::Message* message1,
                         protobuf::Message* message2) {
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

size_t MutateTextMessage(uint8_t* data, size_t size, size_t max_size,
                         unsigned int seed, protobuf::Message* message) {
  TextInputReader input(data, size);
  TextOutputWriter output(data, max_size);
  return MutateMessage(seed, input, &output, message);
}

size_t CrossOverTextMessages(const uint8_t* data1, size_t size1,
                             const uint8_t* data2, size_t size2, uint8_t* out,
                             size_t max_out_size, unsigned int seed,
                             protobuf::Message* message1,
                             protobuf::Message* message2) {
  TextInputReader input1(data1, size1);
  TextInputReader input2(data2, size2);
  TextOutputWriter output(out, max_out_size);
  return CrossOverMessages(seed, input1, input2, &output, message1, message2);
}

size_t MutateBinaryMessage(uint8_t* data, size_t size, size_t max_size,
                           unsigned int seed, protobuf::Message* message) {
  BinaryInputReader input(data, size);
  BinaryOutputWriter output(data, max_size);
  return MutateMessage(seed, input, &output, message);
}

size_t CrossOverBinaryMessages(const uint8_t* data1, size_t size1,
                               const uint8_t* data2, size_t size2, uint8_t* out,
                               size_t max_out_size, unsigned int seed,
                               protobuf::Message* message1,
                               protobuf::Message* message2) {
  BinaryInputReader input1(data1, size1);
  BinaryInputReader input2(data2, size2);
  BinaryOutputWriter output(out, max_out_size);
  return CrossOverMessages(seed, input1, input2, &output, message1, message2);
}

}  // namespace

  size_t CustomProtoMutator(bool binary, uint8_t* data, size_t size,
                            size_t max_size, unsigned int seed,
                            protobuf::Message* input) {
    auto mutate = binary ? &MutateBinaryMessage : &MutateTextMessage;
    return mutate(data, size, max_size, seed, input);
  }

  size_t CustomProtoCrossOver(bool binary, const uint8_t* data1, size_t size1,
                              const uint8_t* data2, size_t size2, uint8_t* out,
                              size_t max_out_size, unsigned int seed,
                              protobuf::Message* input1,
                              protobuf::Message* input2) {
    auto cross = binary ? &CrossOverBinaryMessages : &CrossOverTextMessages;
    return cross(data1, size1, data2, size2, out, max_out_size, seed, input1, input2);
  }

  bool LoadProtoInput(bool binary, const uint8_t* data, size_t size, protobuf::Message* input) {
    if (GetCache()->LoadIfSame(data, size, input)) return true;
    auto result = binary ? ParseBinaryMessage(data, size, input) : ParseTextMessage(data, size, input);
    if (!result) return false;
    GetMutator()->Seed(size);
    GetMutator()->Fix(input);
    return true;
  }

  void RegisterPostProcessor(const protobuf::Descriptor* desc,
      std::function<void(protobuf::Message* message, unsigned int seed)> callback) {
    GetMutator()->RegisterPostProcessor(desc, callback);
  }

}  // namespace protobuf_mutator
