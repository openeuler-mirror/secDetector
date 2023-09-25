cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
project(observer_agent VERSION 1.0 LANGUAGES CXX)
add_executable(secDetectord service/main.cpp service/ringbuffer.cpp)
target_include_directories(secDetectord PUBLIC service)