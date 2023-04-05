option( DEBUG "Set to OFF to disable printing debug messages" ON)

if(DEBUG)
    message("Working in DEBUG mode (print debug messages).")
    add_definitions(-DDEBUG)
endif()


file(GLOB TEST_SRC_LIST "*.cpp")
set(FHE_PROTO_SRC ${CMAKE_SOURCE_DIR}/src/proto/openfhe_ckks.pb.cc)
add_executable(test ${TEST_SRC_LIST} ${FHE_PROTO_SRC})
target_link_libraries(test ${PROTOBUF_LIBRARIES})
target_include_directories(test PUBLIC ${CMAKE_SOURCE_DIR}/src/)
target_include_directories(test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_dependencies(test aflpm)
target_link_libraries(test aflpm)
