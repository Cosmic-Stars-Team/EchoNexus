cmake_minimum_required(VERSION 3.10)

project(EchoNexusExample CXX)

add_executable(example src/main.cpp)

target_include_directories(example PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(example PRIVATE EchoNexus)
