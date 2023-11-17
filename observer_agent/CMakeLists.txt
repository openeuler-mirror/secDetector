cmake_minimum_required(VERSION 3.1)

add_subdirectory(ebpf)
set(CMAKE_CXX_STANDARD 11)
project(observer_agent VERSION 1.0 LANGUAGES CXX)
set(GRPC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/grpc_comm)
add_custom_target(grpc_demo ALL
	COMMAND make -C ${GRPC_PATH})

add_executable(secDetectord grpc_comm/client.cpp grpc_comm/server.cpp service/main.cpp service/ringbuffer.cpp
	                    service/ebpf_converter.cpp)
add_dependencies(secDetectord ebpf)
target_include_directories(secDetectord PUBLIC service grpc_comm ${CMAKE_SOURCE_DIR}/include .)

target_link_libraries(secDetectord ${CMAKE_CURRENT_BINARY_DIR}/ebpf/.output/fentry.o)
target_link_libraries(secDetectord ${CMAKE_CURRENT_BINARY_DIR}/ebpf/file_ebpf/.output/file_fentry.o)
target_link_libraries(secDetectord ${GRPC_PATH}/comm_api.pb.o ${GRPC_PATH}/comm_api.grpc.pb.o)
target_link_libraries(secDetectord protobuf grpc++ grpc absl_synchronization uuid)
target_link_libraries(secDetectord z elf bpf)
