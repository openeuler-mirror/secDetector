cmake_minimum_required(VERSION 3.14.1)

project(secDetector_sdk)

set(CMAKE_CXX_FALAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++ -g")
set(SDK_LINK_FLAGS "--lgrpc++ -lgrpc -lprotobuf -lpthread -wl,--no-as-needed -lgrpc++_reflection -wl,--as-needed -ldl")
set(SDK_SRC secDetector_sdk.cpp ../observer_agent/grpc_comm/comm_api.grpc.pb.o ../observer_agent/grpc_comm/comm_api.pb.o)

add_library(secDetectorsdk SHARED ${SDK_SRC})

set_target_properties(secDetectorsdk PROPERTIES LINK_FALSGS "${SDK_LINK_FLAGS}")
target_include_directories(secDetectorsdk PROVATE ./ ../observer_agent/grpc_comm/)
