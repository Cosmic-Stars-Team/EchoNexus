cmake_minimum_required(VERSION 3.10)

project(EchoNexus LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ECHONEXUS_DISABLE_BEAST "Disable Boost.Beast dependency" OFF)
option(ECHONEXUS_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)

# EchoNexus library sources
set(LIB_SOURCES
        lib/core.cpp
        lib/types/handler.hpp
        lib/types/request.hpp
        lib/types/response.hpp
)

# Find libraries
set(BOOST_COMPS asio system)

if(NOT ECHONEXUS_DISABLE_BEAST)
    list(APPEND BOOST_COMPS beast)
endif()

find_package(Boost REQUIRED CONFIG COMPONENTS ${BOOST_COMPS})
find_package(glaze REQUIRED CONFIG)

# Define EchoNexus library
add_library(EchoNexus ${LIB_SOURCES})

target_compile_features(EchoNexus PUBLIC cxx_std_23)

# Link libraries
target_link_libraries(EchoNexus PUBLIC Boost::asio)
target_link_libraries(EchoNexus PUBLIC glaze::glaze)

if(ECHONEXUS_DISABLE_BEAST)
    message(STATUS "Build: Boost.Beast is DISABLED")
    target_compile_definitions(EchoNexus PUBLIC DISABLE_BOOST_BEAST)
else()
    if(TARGET Boost::beast)
        target_link_libraries(EchoNexus PUBLIC Boost::beast)
    else()
        target_link_libraries(EchoNexus PUBLIC Boost::headers)
    endif()
endif()

target_compile_options(EchoNexus PRIVATE
    # ============================================
    #                  For MSVC
    # ============================================
    $<$<CXX_COMPILER_ID:MSVC>:
        /W4
        /utf-8
        /permissive-
        /Zc:__cplusplus
    >
    # ============================================
    # For Unix-like Compiler (GCC/Clang/AppleClang)
    # ============================================
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    >

    # ============================================
    #              Warning to Error
    # ============================================
    $<$<BOOL:${ECHONEXUS_WARNINGS_AS_ERRORS}>:
        $<$<CXX_COMPILER_ID:MSVC>:/WX>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Werror>
    >
)

target_include_directories(EchoNexus PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# 添加 example 目录
add_subdirectory(example)
