cmake_minimum_required(VERSION 3.14.1)

project(secDetector_sdk)

set(CMAKE_CXX_FALAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++ -g")
set(SDK_SRC secDetector_sdk.cpp ../observer_agent/grpc_comm/comm_api.grpc.pb.o ../observer_agent/grpc_comm/comm_api.pb.o ../observer_agent/grpc_comm/client.cpp)

add_library(secDetectorsdk SHARED ${SDK_SRC})

target_include_directories(secDetectorsdk PRIVATE ./ ../observer_agent/grpc_comm/)
target_link_libraries(secDetectorsdk PRIVATE grpc++ grpc protobuf pthread grpc++_reflection dl uuid)
