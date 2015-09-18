cmake_minimum_required(VERSION 3.2)
project(mongo_fw)

# Version number
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
string(TIMESTAMP BUILD_ID "%Y%m%d" UTC)


# Mesos installation directory
set(MESOS_INSTALL_DIR $ENV{MESOS_INSTALL})
if(NOT MESOS_INSTALL_DIR)
    set(MESOS_INSTALL_DIR /usr/local)
endif(NOT MESOS_INSTALL_DIR)

include_directories(SYSTEM ${MESOS_INSTALL_DIR}/include)
link_directories(${MESOS_INSTALL_DIR}/lib)

# Adding gtest (TODO: add gmock too)
include_directories(${MESOS_INSTALL_DIR}/gtest-1.7.0/include)

# Directories configuration
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(GEN_DIR ${PROJECT_SOURCE_DIR}/gen)
set(CMAKE_PREFIX_PATH /usr/local)
set(PROTO_DIR ${PROJECT_SOURCE_DIR}/proto)

message("Building project `${PROJECT_NAME}`
         from: ${PROJECT_SOURCE_DIR}
         to: ${PROJECT_BINARY_DIR}")

# Configure a header file to pass settings into source code
configure_file(
    ${PROJECT_SOURCE_DIR}/config.h.in
    ${GEN_DIR}/config.h
)


include_directories(${SOURCE_DIR}/include
                    ${GEN_DIR}
                    ${PROJECT_BINARY_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Build protocol buffers
add_custom_command(
    OUTPUT ${GEN_DIR}/mongo.pb.cc ${GEN_DIR}/mongo.pb.h
    COMMAND protoc -I=${PROTO_DIR} --cpp_out=${GEN_DIR} ${PROTO_DIR}/mongo.proto
    DEPENDS ${PROTO_DIR}/mongo.proto
)

set(SOURCE_FILES ${SOURCE_DIR}/main.cpp
                 ${SOURCE_DIR}/mongo_scheduler.cpp
                 ${GEN_DIR}/mongo.pb.cc
)

add_executable(mongo_fw ${SOURCE_FILES})
target_link_libraries(mongo_fw mesos gtest -lpthread -lm)
