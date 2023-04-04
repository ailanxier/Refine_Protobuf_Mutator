# add_subdirectory(Fuzzer)

set(LIBAFLPM_SRC afl_mutator.cpp)
add_library(aflpm STATIC ${LIBAFLPM_SRC})
target_link_libraries(aflpm protobuf-mutator)
# target_link_libraries(aflpm Fuzzer)

add_subdirectory(proto)
set(FHE_PROTO_SRC proto/fhe.pb.cc)
set(POSTPROCESS_SRC fhe_protobuf_mutator.cpp)
add_library(fhe_protobuf_mutator SHARED ${POSTPROCESS_SRC} ${FHE_PROTO_SRC})
target_include_directories(fhe_protobuf_mutator PUBLIC proto)
add_dependencies(fhe_protobuf_mutator aflpm)
target_link_libraries(fhe_protobuf_mutator aflpm)