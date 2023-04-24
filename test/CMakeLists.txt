file(GLOB CREATE_SRC_LIST "test.cpp")

add_executable(performanceTest ${CREATE_SRC_LIST} ${CMAKE_SOURCE_DIR}/test/xml.pb.cc)
target_link_libraries(performanceTest ${PROTOBUF_LIBRARIES})
target_include_directories(performanceTest PUBLIC ${CMAKE_SOURCE_DIR}/postprocess)
# target_include_directories(performanceTest PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
add_dependencies(performanceTest ${CUSTOM_MUTATOR_NAME})
target_link_libraries(performanceTest ${CUSTOM_MUTATOR_NAME})