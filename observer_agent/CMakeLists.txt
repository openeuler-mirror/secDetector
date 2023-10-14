cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 11)
project(observer_agent VERSION 1.0 LANGUAGES CXX)
set(GRPC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/grpc_comm)
add_custom_target(grpc_demo ALL
	COMMAND make -C ${GRPC_PATH})
add_executable(secDetectord service/main.cpp service/ringbuffer.cpp)
target_include_directories(secDetectord PUBLIC service)