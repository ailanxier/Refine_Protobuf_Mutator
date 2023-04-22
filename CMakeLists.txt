CMAKE_MINIMUM_REQUIRED(VERSION 3.10)
PROJECT(refine-protobuf-mutator CXX)

enable_language(C)
enable_language(CXX)

# set c++ version
set (CMAKE_CXX_STANDARD 17)

set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# External dependencies
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# check -fPIC
set(CMAKE_REQUIRED_FLAGS "-fPIC")
check_cxx_compiler_flag(-fPIC PIC)
unset(CMAKE_REQUIRED_FLAGS)

if (PIC)
    set(EXTRA_FLAGS "${EXTRA_FLAGS} -fPIC")
endif()

set(C_EXTRA_FLAGS "${C_EXTRA_FLAGS} ${EXTRA_FLAGS}")
set(CXX_EXTRA_FLAGS "${CXX_EXTRA_FLAGS} ${EXTRA_FLAGS}")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_EXTRA_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")

set(PROTOBUF_CFLAGS "${CMAKE_C_FLAGS} ${NO_FUZZING_FLAGS} -w")
set(PROTOBUF_CXXFLAGS "${CMAKE_CXX_FLAGS} ${NO_FUZZING_FLAGS} -w")
if(CMAKE_USE_PTHREADS_INIT)
  set(PROTOBUF_CFLAGS "${PROTOBUF_CFLAGS} -pthread")
  set(PROTOBUF_CXXFLAGS "${PROTOBUF_CXXFLAGS} -pthread")
endif()

option(DEBUG "Set to OFF to disable printing debug messages" ON)

if(DEBUG)
    message("=============== Working in DEBUG mode (print debug messages). ===============")
    add_definitions(-DDEBUG)
endif()

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
message("PROTOBUF_INCLUDE_DIRS: ${PROTOBUF_INCLUDE_DIRS}")
include_directories(${CMAKE_CURRENT_BINARY_DIR})
# add_compile_options(-fsanitize=address -g)
# link_libraries(asan)
# ===================== OPENFHE-CKKS =====================
set(CUSTOM_MUTATOR_NAME "openfhe_ckks_protobuf_mutator")
add_subdirectory(protobuf_mutator)
add_subdirectory(postprocess)
add_subdirectory(mutation_test)
add_subdirectory(proto_seed)
link_libraries(protobuf)







