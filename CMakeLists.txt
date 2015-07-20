cmake_minimum_required(VERSION 3.2)
project(mongo_fw)

# Version number
set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)

# Configure a header file to pass settings into source code
configure_file(
    "${PROJECT_SOURCE_DIR}/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

include_directories("${PROJECT_BINARY_DIR}")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/include/mongo_executor.hpp
    src/include/mongo_scheduler.hpp
    src/main.cpp
    src/mongo_executor.cpp
    src/mongo_scheduler.cpp)

add_executable(mongo_fw ${SOURCE_FILES})
