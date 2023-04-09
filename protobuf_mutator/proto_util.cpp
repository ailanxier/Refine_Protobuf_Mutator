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

    int MutateMessage(const InputReader& input, OutputWriter* output, Message* message) {
        input.Read(message);
        int max_size = output->size(), test_size = max_size;
        GetMutator()->Mutate(message, max_size);
        if (int new_size = output->Write(*message)) {
            // cout<<"mutate:"<<new_size<<" "<<max_size<<endl;
            assert(new_size <= test_size);
            GetCache()->Store(output->data(), new_size, message);
            return new_size;
        }
        return 0;
    }

    int CrossOverMessages(const InputReader& input1, const InputReader& input2, 
                            OutputWriter* output, Message* message1, Message* message2) {
        input1.Read(message1);
        input2.Read(message2);
        int max_size = output->size(), test_size = max_size;
        GetMutator()->Crossover(message1, message2, max_size);
        if (int new_size = output->Write(*message1)) {
            // cout<<"cross:"<<new_size<<" "<<max_size<<endl;
            assert(new_size <= test_size);
            GetCache()->Store(output->data(), new_size, message1);
            return new_size;
        }
        return 0;
    }

    int CustomProtoMutate(bool binary, uint8_t* data, int size, int max_size, Message* message) {
        if(binary) {
            BinaryInputReader b_input(data, size);
            BinaryOutputWriter b_output(data, max_size);
            return MutateMessage(b_input, &b_output, message);
        } else {
            TextInputReader t_input(data, size);
            TextOutputWriter t_output(data, max_size);
            return MutateMessage(t_input, &t_output, message);
        }
    }

    int CustomProtoCrossOver(bool binary, const uint8_t* data1, int size1, const uint8_t* data2, int size2, 
                    uint8_t* out, int max_out_size, Message* message1, Message* message2) {
        if(binary){
            BinaryInputReader b_input1(data1, size1);
            BinaryInputReader b_input2(data2, size2);
            BinaryOutputWriter b_output(out, max_out_size);
            return CrossOverMessages(b_input1, b_input2, &b_output, message1, message2);
        } else {
            TextInputReader t_input1(data1, size1);
            TextInputReader t_input2(data2, size2);
            TextOutputWriter t_output(out, max_out_size);
            return CrossOverMessages(t_input1, t_input2, &t_output, message1, message2);
        }
    }

    bool LoadProtoInput(bool binary, const uint8_t* data, int size, Message* input) {
        if (GetCache()->LoadIfSame(data, size, input)) return true;
        return binary ? ParseBinaryMessage(data, size, input) : ParseTextMessage(data, size, input);
    }

    bool ParseBinaryMessage(const string& data, Message* output) {
        output->Clear();
        if (!output->ParseFromString(data)) {
            output->Clear();
            return false;
        }
        return true;
    }

    bool ParseBinaryMessage(const uint8_t* data, int size, Message* output) {
        return ParseBinaryMessage({(char*)(data), (size_t)size}, output);
    }

    string SaveMessageAsBinary(const Message& message) {
        string tmp;
        if (!message.SerializeToString(&tmp)) return {};
        return tmp;
    }

    int SaveMessageAsBinary(const Message& message, uint8_t* data, int max_size) {
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
        parser.AllowUnknownField(true);
        if (!parser.ParseFromString(data, output)) {
            output->Clear();
            return false;
        }
        return true;
    }

    bool ParseTextMessage(const uint8_t* data, int size, Message* output) {
        return ParseTextMessage({reinterpret_cast<const char*>(data), (size_t)size}, output);
    }

    string SaveMessageAsText(const Message& message) {
        string tmp;
        if (!TextFormat::PrintToString(message, &tmp)) return {};
        return tmp;
    }

    int SaveMessageAsText(const Message& message, uint8_t* data, int max_size) {
        string result = SaveMessageAsText(message);
        if (result.size() <= max_size) {
            memcpy(data, result.data(), result.size());
            return result.size();
        }
        return 0;
    }

}  // namespace protobuf_mutator
