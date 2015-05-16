cmake_minimum_required(VERSION 3.2)
project(mongo_fw)

set(BASE_DIR /Users/marco/workspace/mongo_fw)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_DIR src)
set(CMAKE_PREFIX_PATH /usr/local)
set(CMAKE_INCLUDE_PATH ${BASE_DIR}/src/include)

set(SOURCE_FILES ${SOURCE_DIR}/main.cpp ${SOURCE_DIR}/mongo_scheduler.cpp)

add_executable(mongo_fw ${SOURCE_FILES})
target_link_libraries(mongo_fw mesos gtest)