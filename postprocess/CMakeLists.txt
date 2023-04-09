# 设置后处理执行文件和对应的 protobuf 格式文件
set(POSTPROCESS_SRC postprocess.cpp)
set(PROTO_SRC ${CMAKE_SOURCE_DIR}/proto/openfhe_ckks.pb.cc)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(custom_protobuf_mutator SHARED ${POSTPROCESS_SRC} ${PROTO_SRC})
# target_include_directories(custom_protobuf_mutator PUBLIC ${CMAKE_SOURCE_DIR}/proto)
target_include_directories(custom_protobuf_mutator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(custom_protobuf_mutator protobuf-mutator)
