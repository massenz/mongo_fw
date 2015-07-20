cmake_minimum_required(VERSION 3.2)
project(mongo_fw)

# Version number
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)

# Directories configuration
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(PROJECT_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(GEN_DIR ${PROJECT_SOURCE_DIR}/gen)
set(CMAKE_PREFIX_PATH /usr/local)

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


set(SOURCE_FILES ${SOURCE_DIR}/main.cpp
                 ${SOURCE_DIR}/mongo_scheduler.cpp
)

add_executable(mongo_fw ${SOURCE_FILES})
target_link_libraries(mongo_fw mesos gtest -lpthread -lm)
