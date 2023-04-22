#pragma once

#include "openfhe_bgv.pb.h"
using Root = OpenFHE::OpenFHE_RootMsg;
#define MAX_BINARY_INPUT_SIZE (1500)
#define MAX_BUFFER_SIZE (2000)
#define USE_SEED 0
const std::string afl_seed_path = "/root/Refine_Protobuf_Mutator/proto_seed/afl_text_in/openfhe_bgv/";
const std::string text_seed_path = "/root/Refine_Protobuf_Mutator/proto_seed/in/";
const std::string bin_seed_path = "/root/Refine_Protobuf_Mutator/proto_seed/bin/";
