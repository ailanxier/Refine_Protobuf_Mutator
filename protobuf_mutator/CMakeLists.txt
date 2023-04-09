# add_subdirectory(Fuzzer)

file(GLOB LPM_SRC_LIST "*.cpp")
add_library(protobuf-mutator STATIC ${LPM_SRC_LIST})
target_link_libraries(protobuf-mutator ${PROTOBUF_LIBRARIES})
set_target_properties(protobuf-mutator PROPERTIES COMPILE_FLAGS "${NO_FUZZING_FLAGS}" SOVERSION 0)
# target_link_libraries(protobuf-mutator Fuzzer)
