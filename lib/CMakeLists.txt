cmake_minimum_required(VERSION 3.14.1)

project(secDetector_sdk)

set(CMAKE_CXX_FALAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++ -g")
set(SDK_SRC secDetector_sdk.cpp  ${CMAKE_SOURCE_DIR}/observer_agent/grpc_comm/client.cpp)

add_library(secDetectorsdk SHARED ${SDK_SRC})
add_dependencies(secDetectorsdk secDetectord)

target_include_directories(secDetectorsdk PRIVATE ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/observer_agent/grpc_comm/)
target_link_libraries(secDetectorsdk PRIVATE grpc++ grpc protobuf pthread grpc++_reflection dl uuid)
target_link_libraries(secDetectorsdk PRIVATE ${CMAKE_SOURCE_DIR}/observer_agent/grpc_comm/comm_api.grpc.pb.o ${CMAKE_SOURCE_DIR}/observer_agent/grpc_comm/comm_api.pb.o)
