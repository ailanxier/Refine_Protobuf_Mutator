#include "proto_util.h"
#include "mutator.h"
namespace protobuf_mutator {

    LastMutationCache* GetCache() {
        static LastMutationCache cache;
        return &cache;
    }

    Mutator* GetMutator() {
        static Mutator mutator;
        return &mutator;
    }


    size_t MutateMessage(unsigned int seed, const InputReader& input, OutputWriter* output, Message* message) {
        GetMutator()->Seed(seed);
        input.Read(message);
        size_t max_size = output->size();
        GetMutator()->Mutate(message, max_size);
        if (size_t new_size = output->Write(*message)) {
            assert(new_size <= max_size);
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
        size_t max_size = output->size();
        GetMutator()->Crossover(*message2, message1, max_size);
        if (size_t new_size = output->Write(*message1)) {
        assert(new_size <= max_size);
        GetCache()->Store(output->data(), new_size, message1);
        return new_size;
        }
        return 0;
    }

    size_t CustomProtoMutate(bool binary, uint8_t* data, size_t size, size_t max_size, unsigned int seed,
                                Message* message) {
        if(binary) {
            BinaryInputReader b_input(data, size);
            BinaryOutputWriter b_output(data, max_size);
            return MutateMessage(seed, b_input, &b_output, message);
        } else {
            TextInputReader t_input(data, size);
            TextOutputWriter t_output(data, max_size);
            return MutateMessage(seed, t_input, &t_output, message);
        }
    }

    size_t CustomProtoCrossOver(bool binary, const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2, 
                    uint8_t* out, size_t max_out_size, unsigned int seed, Message* message1, Message* message2) {
        if(binary){
            BinaryInputReader b_input1(data1, size1);
            BinaryInputReader b_input2(data2, size2);
            BinaryOutputWriter b_output(out, max_out_size);
            return CrossOverMessages(seed, b_input1, b_input2, &b_output, message1, message2);
        } else {
            TextInputReader t_input1(data1, size1);
            TextInputReader t_input2(data2, size2);
            TextOutputWriter t_output(out, max_out_size);
            return CrossOverMessages(seed, t_input1, t_input2, &t_output, message1, message2);
        }
    }

    bool LoadProtoInput(bool binary, const uint8_t* data, size_t size, Message* input) {
        if (GetCache()->LoadIfSame(data, size, input)) return true;
        auto result = binary ? ParseBinaryMessage(data, size, input) : ParseTextMessage(data, size, input);
        if (!result) return false;
        return true;
    }


    bool ParseBinaryMessage(const string& data, Message* output) {
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

    string SaveMessageAsBinary(const Message& message) {
        string tmp;
        if (!message.SerializePartialToString(&tmp)) return {};
        return tmp;
    }

    size_t SaveMessageAsBinary(const Message& message, uint8_t* data, size_t max_size) {
        string result = SaveMessageAsBinary(message);
        if (result.size() <= max_size) {
            memcpy(data, result.data(), result.size());
            return result.size();
        }
        return 0;
    }

    bool ParseTextMessage(const string& data, Message* output) {
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

    string SaveMessageAsText(const Message& message) {
        string tmp;
        if (!TextFormat::PrintToString(message, &tmp)) return {};
        return tmp;
    }

    size_t SaveMessageAsText(const Message& message, uint8_t* data, size_t max_size) {
        string result = SaveMessageAsText(message);
        if (result.size() <= max_size) {
            memcpy(data, result.data(), result.size());
            return result.size();
        }
        return 0;
    }

}  // namespace protobuf_mutator
