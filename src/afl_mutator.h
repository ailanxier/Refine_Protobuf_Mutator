#ifndef AFLPLUSPLUS_PROTOBUF_MUTATOR_SRC_AFL_MUTATE_H_
#define AFLPLUSPLUS_PROTOBUF_MUTATOR_SRC_AFL_MUTATE_H_

#include <iostream>
#include <fstream>
#include <tuple>
#include "Fuzzer/FuzzerDefs.h"
#include "Fuzzer/FuzzerRandom.h"
#include "Fuzzer/FuzzerMutate.h"
#include "Fuzzer/FuzzerInternal.h"
#include "Fuzzer/FuzzerCorpus.h"
#include "Fuzzer/FuzzerExtFunctions.h"
#include "protobuf_mutator/mutator.h"

template <typename T>
struct first_argument_type;

template <typename Ret, typename... Args>
struct first_argument_type<Ret(Args...)>{
    using argument_type = std::tuple<Args...>;
    using type = typename std::remove_cv_t<std::remove_reference_t<std::tuple_element_t<0, argument_type>>>;
};

// Defines custom mutator, crossover and test functions using default serialization format. Default is text.
#define DEFINE_AFL_PROTO_FUZZER(args...) DEFINE_AFL_BINARY_PROTO_FUZZER(args)
// Defines custom mutator, crossover and test functions using text serialization. 
#define DEFINE_AFL_TEXT_PROTO_FUZZER(args...)   DEFINE_AFL_PROTO_FUZZER_IMPL(false, args)
#define DEFINE_AFL_BINARY_PROTO_FUZZER(args...) DEFINE_AFL_PROTO_FUZZER_IMPL(true, args)


// 最终实现的是 ProtoToDataHelper 函数
#define DEFINE_AFL_PROTO_FUZZER_IMPL(use_binary, args...)                                                 \
  static size_t ProtoToDataHelper(args);                                                                  \
  using protobuf_mutator::aflplusplus::MutateHelper;                                                      \
  using FuzzerProtoType = first_argument_type<decltype(ProtoToDataHelper)>::type;                         \
  DEFINE_AFL_CUSTOM_INIT                                                                                  \
  DEFINE_AFL_CUSTOM_DEINIT                                                                                \
  DEFINE_AFL_CUSTOM_PROTO_MUTATOR_IMPL(use_binary, FuzzerProtoType)                                       \
  DEFINE_AFL_CUSTOM_PROTO_POST_PROCESS_IMPL(use_binary, FuzzerProtoType)                                  \
  static size_t ProtoToDataHelper(args)

#define DEFINE_AFL_CUSTOM_INIT  extern "C"                                                               \
  MutateHelper *afl_custom_init(void *afl, unsigned int s){                                              \
      MutateHelper *mutate_helper = new MutateHelper(s);                                                 \
      std::ofstream seed_of("seed.txt", std::ios::trunc);                                                \
      seed_of << s << std::endl;                                                                         \
      seed_of.close();                                                                                   \
      return mutate_helper;                                                                              \
  } 

// Deinitialize everything  
#define DEFINE_AFL_CUSTOM_DEINIT  extern "C"                                                             \
  void afl_custom_deinit(MutateHelper *m){                                                               \
      delete m;                                                                                          \
  }

// Implementation of macros above.
#define DEFINE_AFL_CUSTOM_PROTO_MUTATOR_IMPL(use_binary, Proto) extern "C"                               \
  size_t afl_custom_fuzz(MutateHelper *m, unsigned char *buf, size_t buf_size, unsigned char **out_buf,  \
                    unsigned char *add_buf, size_t add_buf_size, size_t max_size) {                      \
    Proto input1;                                                                                        \
    Proto input2;                                                                                        \
    return AFL_CustomProtoMutator(m, use_binary, buf, buf_size, out_buf,                                 \
                                add_buf, add_buf_size, max_size, &input1, &input2);                      \
  }

// A post-processing function to use right before AFL++ writes the test case to disk in order to execute the target.
#define DEFINE_AFL_CUSTOM_PROTO_POST_PROCESS_IMPL(use_binary, Proto)  extern "C"                                    \
  size_t afl_custom_post_process(MutateHelper *m, unsigned char *buf, size_t buf_size, unsigned char **out_buf) {   \
    using protobuf_mutator::LoadProtoInput;                                                                         \
    Proto input;                                                                                                    \
    if (LoadProtoInput(use_binary, buf, buf_size, &input))                                                          \
      return ProtoToDataHelper(input, out_buf, m->GetIndex());                                                      \
    return 0;                                                                                                       \
  }


namespace protobuf_mutator{
  namespace aflplusplus {
    // Embedding buf_ in class MutateHelper here to prevent memory fragmentation caused by frequent memory allocation.
    class MutateHelper {
      public: 
          size_t GetSeed() const { return seed_; }
          size_t GetLen() const { return len_; }
          void SetLen(size_t len) { len_ = len; }
          uint8_t* GetOutBuf() { return buf_; }
          uint8_t* ReallocBuf(size_t len);
          size_t GetIndex() const { return index; }
          void SetIndex(size_t i) { index = i; }
          MutateHelper(size_t s);
          ~MutateHelper() = default;
          
          
      private:
          size_t seed_;
          uint8_t *buf_;  // 每次 afl_custom_fuzz() 的 out_buf
          size_t len_;
          size_t index = 1;
    };

    //Implementation of Crossover and Mutation of test cases in A(FL)_CustomProtoMutator.
    size_t AFL_CustomProtoMutator(MutateHelper *m, bool binary, unsigned char *buf, size_t buf_size, 
                                unsigned char **out_buf, unsigned char *add_buf, 
                                size_t add_buf_size, size_t max_size, Message* input1, Message* input2);

  } // namespace aflplusplus
} // namespace protobuf_mutator


#endif // AFLPLUSPLUS_PROTOBUF_MUTATOR_SRC_AFL_MUTATE_H_
// Initialize this custom mutator   
// #define DEFINE_AFL_CUSTOM_INIT  extern "C"                                                               \
//   MutateHelper *afl_custom_init(void *afl, unsigned int s){                                              \
//       using namespace fuzzer;                                                                            \
//       MutateHelper *mutate_helper = new MutateHelper(s);                                                 \
//       std::ofstream seed_of("seed.txt", std::ios::trunc);                                                \
//       seed_of << s << std::endl;                                                                         \
//       seed_of.close();                                                                                   \
//       auto seed = Random(s);                                                                             \
//       FuzzingOptions Options;                                                                            \
//       std::unique_ptr<ExternalFunctions> t(new ExternalFunctions());                                     \
//       EF = t.get();                                                                                      \
//       auto *MD = new MutationDispatcher(seed, Options);                                                  \
//       static auto *Corpus = new InputCorpus("");                                                         \
//       auto *F = new Fuzzer((UserCallback)nullptr, *Corpus, *MD, Options);                                \
//       return mutate_helper;                                                                              \
//   } 