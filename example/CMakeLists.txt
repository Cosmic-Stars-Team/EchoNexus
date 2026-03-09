cmake_minimum_required(VERSION 3.23)

add_executable(example src/main.cpp)

target_link_libraries(example PRIVATE EchoNexus)
