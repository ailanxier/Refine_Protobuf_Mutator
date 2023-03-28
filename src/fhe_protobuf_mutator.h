#ifndef FHE_PROTOBUF_MUTATOR_H_
#define FHE_PROTOBUF_MUTATOR_H_

#include "proto/fhe.pb.h"
#include "afl_mutator.h"

namespace fhe_protobuf_mutator {
    class TestMessageHandler{
        public:
            TestMessageHandler();
            ~TestMessageHandler();
            size_t TransferMessageType(const Root& input, unsigned char **out_buf);
            std::ofstream of;
            char *temp;
    };
} //namespace test_fuzzer

// // Callback to postprocess mutations.
// // Implementation should use seed to initialize random number generators.
// using PostProcess = std::function<void(Message* message, unsigned int seed)>;

// // Register callback which will be called after every message mutation.
// // In this callback fuzzer may adjust content of the message or mutate some
// // fields in some fuzzer specific way.
// void RegisterPostProcessor(const Descriptor* desc, PostProcess callback);
// class PostProcessing {
//   public:
//     using PostProcessors = std::unordered_multimap<const Descriptor*, Mutator::PostProcess>;

//     PostProcessing(bool keep_initialized, const PostProcessors& post_processors, RandomEngine* random)
//         : keep_initialized_(keep_initialized), post_processors_(post_processors), random_(random) {}

//     void Run(Message* message, int max_depth) {
//       --max_depth;
//       const Descriptor* descriptor = message->GetDescriptor();
//       const Reflection* reflection = message->GetReflection();
//       for (int i = 0; i < descriptor->field_count(); i++) {
//         const FieldDescriptor* field = descriptor->field(i);
//         if (keep_initialized_ && (field->is_required() || descriptor->options().map_entry()) &&
//             !reflection->HasField(*message, field)) {
//           CreateDefaultField()(FieldInstance(message, field));
//         }

//         if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) continue;

//         if (max_depth < 0 && !field->is_required()) {
//           // Clear deep optional fields to avoid stack overflow.
//           reflection->ClearField(message, field);
//           if (field->is_repeated())
//             assert(!reflection->FieldSize(*message, field));
//           else
//             assert(!reflection->HasField(*message, field));
//           continue;
//         }

//         if (field->is_repeated()) {
//           const int field_size = reflection->FieldSize(*message, field);
//           for (int j = 0; j < field_size; ++j) {
//             Message* nested_message = reflection->MutableRepeatedMessage(message, field, j);
//             Run(nested_message, max_depth);
//           }
//         } else if (reflection->HasField(*message, field)) {
//           Message* nested_message = reflection->MutableMessage(message, field);
//           Run(nested_message, max_depth);
//         }
//       }

//       // Call user callback after message trimmed, initialized and packed.
//       auto range = post_processors_.equal_range(descriptor);
//       for (auto it = range.first; it != range.second; ++it)
//         it->second(message, (*random_)());
//     }

//   private:
//     bool keep_initialized_;
//     const PostProcessors& post_processors_;
//     RandomEngine* random_;
//   };
//   void Mutator::RegisterPostProcessor(const Descriptor* desc, PostProcess callback) {
//     post_processors_.emplace(desc, callback);
//   }
//   void RegisterPostProcessor(const Descriptor* desc,
//       std::function<void(Message* message, unsigned int seed)> callback) {
//     GetMutator()->RegisterPostProcessor(desc, callback);
//   }
// using PostProcessors = std::unordered_multimap<const Descriptor*, PostProcess>;
// PostProcessors post_processors_;
#endif