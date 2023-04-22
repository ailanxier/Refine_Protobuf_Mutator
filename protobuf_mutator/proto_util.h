#pragma once

#include <stddef.h>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <random>
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <bitset>
#include "google/protobuf/any.pb.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/message.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/util/message_differencer.h"
#include "google/protobuf/wire_format.h"


namespace protobuf_mutator {
    namespace protobuf = google::protobuf;
    using std::string;
    using std::vector;
    using std::bitset;
    using protobuf::Message;
    using protobuf::Descriptor;
    using protobuf::TextFormat;
    using protobuf::FieldDescriptor;
    using protobuf::FileDescriptor;
    using protobuf::OneofDescriptor;
    using protobuf::Reflection;
    using protobuf::util::MessageDifferencer;
    int CustomProtoMutate(bool binary, uint8_t* data, int size, int max_size, Message* input);
    int CustomProtoCrossOver(bool binary, const uint8_t* data1, int size1, const uint8_t* data2, 
            int size2, uint8_t* out, int max_out_size, Message* input1, Message* input2);

    // data -> input
    bool LoadProtoInput(bool binary, const uint8_t* data, int size, Message* input);
    
    bool ParseTextMessage(const uint8_t* data, int size, Message* output);
    bool ParseTextMessage(const string& data, Message* output);
    int SaveMessageAsText(const Message& message, uint8_t* data, int max_size);
    string SaveMessageAsText(const Message& message);
    bool ParseBinaryMessage(const uint8_t* data, int size, Message* output);
    bool ParseBinaryMessage(const string& data, Message* output);
    int SaveMessageAsBinary(const Message& message, uint8_t* data, int max_size);
    string SaveMessageAsBinary(const Message& message);

    class InputReader {
    public:
        InputReader(const uint8_t* data, int size) : data_(data), size_(size) {}
        virtual ~InputReader() = default;

        virtual bool Read(Message* message) const = 0;

        const uint8_t* data() const { return data_; }
        int size() const { return size_; }

    private:
        const uint8_t* data_;
        int size_;
    };

    class OutputWriter {
    public:
        OutputWriter(uint8_t* data, int size) : data_(data), size_(size) {}
        virtual ~OutputWriter() = default;

        virtual int Write(const Message& message) = 0;

        uint8_t* data() const { return data_; }
        int size() const { return size_; }

    private:
        uint8_t* data_;
        int size_;
    };

    class TextInputReader : public InputReader {
    public:
        using InputReader::InputReader;
        bool Read(Message* message) const override { return ParseTextMessage(data(), size(), message); }
    };

    class TextOutputWriter : public OutputWriter {
        public:
        using OutputWriter::OutputWriter;
        int Write(const Message& message) override { return SaveMessageAsText(message, data(), size()); }
    };

    class BinaryInputReader : public InputReader {
        public:
        using InputReader::InputReader;
        bool Read(Message* message) const override { return ParseBinaryMessage(data(), size(), message); }
    };

    class BinaryOutputWriter : public OutputWriter {
    public:
        using OutputWriter::OutputWriter;
        int Write(const Message& message) override { return SaveMessageAsBinary(message, data(), size()); }
    };

    class LastMutationCache {
    public:
        void Store(const uint8_t* data, int size, Message* message) {
            if (!message_) message_.reset(message->New());
            message->GetReflection()->Swap(message, message_.get());
            data_.assign(data, data + size);
        }

        bool LoadIfSame(const uint8_t* data, int size, Message* message) {
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
}  // namespace protobuf_mutator

