# 设置后处理执行文件和对应的 protobuf 格式文件
set(POSTPROCESS_SRC postprocess.cpp)
set(PROTO_SRC ${CMAKE_SOURCE_DIR}/proto/openfhe_bgv.pb.cc)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

add_library(${CUSTOM_MUTATOR_NAME} SHARED ${POSTPROCESS_SRC} ${PROTO_SRC})
target_include_directories(${CUSTOM_MUTATOR_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${CUSTOM_MUTATOR_NAME} protobuf-mutator)
# add_custom_command(TARGET ${CUSTOM_MUTATOR_NAME} POST_BUILD
#                    COMMAND ${CMAKE_COMMAND} -E copy
#                        $<TARGET_FILE:${CUSTOM_MUTATOR_NAME}> /root/FHE_test/afl/lib)