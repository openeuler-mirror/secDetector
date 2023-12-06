cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 11)
# c++版本

# 设置项目名称
project(resourcesusage)

# 添加头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# 获取当前目录下的所有.h和.hh文件
file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h ${CMAKE_CURRENT_SOURCE_DIR}/*.hh)

# 获取当前目录下的所有.cc和.cpp文件
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cc ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Add main.cpp to the SOURCE_FILES variable
add_executable(resourcesusage ${SOURCE_FILES})

# 添加pthread库链接
find_package(Threads REQUIRED)
target_link_libraries(resourcesusage Threads::Threads)