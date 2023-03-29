#ifndef PROTO_UTIL_H_
#define PROTO_UTIL_H_

#include <stddef.h>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <random>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>

#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "google/protobuf/wire_format.h"

namespace protobuf_mutator {
  namespace protobuf = google::protobuf;
  using String = std::string;
  using protobuf::Message;
  using protobuf::Descriptor;
  using protobuf::TextFormat;
  using protobuf::FieldDescriptor;
  using protobuf::FileDescriptor;
  using protobuf::OneofDescriptor;
  using protobuf::Reflection;
  using protobuf::util::MessageDifferencer;
  using RandomEngine = std::minstd_rand;
  size_t CustomProtoMutate(bool binary, uint8_t* data, size_t size, size_t max_size, unsigned int seed, Message* input);
  size_t CustomProtoCrossOver(bool binary, const uint8_t* data1, size_t size1, const uint8_t* data2, 
        size_t size2, uint8_t* out, size_t max_out_size, unsigned int seed, Message* input1, Message* input2);

  // data -> input
  bool LoadProtoInput(bool binary, const uint8_t* data, size_t size, Message* input);
  
  bool ParseTextMessage(const uint8_t* data, size_t size, Message* output);
  bool ParseTextMessage(const String& data, Message* output);
  size_t SaveMessageAsText(const Message& message, uint8_t* data, size_t max_size);
  String SaveMessageAsText(const Message& message);
  bool ParseBinaryMessage(const uint8_t* data, size_t size, Message* output);
  bool ParseBinaryMessage(const String& data, Message* output);
  size_t SaveMessageAsBinary(const Message& message, uint8_t* data, size_t max_size);
  String SaveMessageAsBinary(const Message& message);

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
    bool Read(Message* message) const override { return ParseTextMessage(data(), size(), message); }
  };

  class TextOutputWriter : public OutputWriter {
    public:
      using OutputWriter::OutputWriter;
      size_t Write(const Message& message) override { return SaveMessageAsText(message, data(), size()); }
  };

  class BinaryInputReader : public InputReader {
    public:
      using InputReader::InputReader;
      bool Read(Message* message) const override { return ParseBinaryMessage(data(), size(), message); }
  };

  class BinaryOutputWriter : public OutputWriter {
    public:
      using OutputWriter::OutputWriter;
      size_t Write(const Message& message) override { return SaveMessageAsBinary(message, data(), size()); }
  };

  class LastMutationCache {
    public:
      void Store(const uint8_t* data, size_t size, Message* message) {
        if (!message_) message_.reset(message->New());
        message->GetReflection()->Swap(message, message_.get());
        data_.assign(data, data + size);
      }

      bool LoadIfSame(const uint8_t* data, size_t size, Message* message) {
        if (!message_ || size != data_.size() || !std::equal(data_.begin(), data_.end(), data))
          return false;

        message->GetReflection()->Swap(message, message_.get());
        message_.reset();
        return true;
      }

    private:
      std::vector<uint8_t> data_;
      std::unique_ptr<Message> message_;
  };

  // Algorithm pick one item from the sequence of weighted items.
  // https://en.wikipedia.org/wiki/Reservoir_sampling#Algorithm_A-Chao
  //
  // Example:
  //   WeightedReservoirSampler<int> sampler;
  //   for(int i = 0; i < size; ++i)
  //     sampler.Pick(weight[i], i);
  //   return sampler.GetSelected();
  template <class T, class RandomEngine = std::default_random_engine>
  class WeightedReservoirSampler {
    public:
      explicit WeightedReservoirSampler(RandomEngine* random) : random_(random) {}

      void Try(uint64_t weight, const T& item) { if (Pick(weight)) selected_ = item; }

      const T& selected() const { return selected_; }

      bool IsEmpty() const { return total_weight_ == 0; }

    private:
      bool Pick(uint64_t weight) {
        if (weight == 0) return false;
        total_weight_ += weight;
        return weight == total_weight_ || 
          std::uniform_int_distribution<uint64_t>(1, total_weight_)(*random_) <= weight;
      }

      T selected_ = {};
      uint64_t total_weight_ = 0;
      RandomEngine* random_;
  };
}  // namespace protobuf_mutator

#endif  // SRC_LIBFUZZER_LIBFUZZER_MACRO_H_
