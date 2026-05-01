cmake_minimum_required(VERSION 3.25)

# Top-level builds can choose or bootstrap a vcpkg toolchain before project().
set(ECHONEXUS_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(ECHONEXUS_IS_TOP_LEVEL ON)
endif()

option(
    ECHONEXUS_AUTO_SETUP_VCPKG
    "Auto clone/bootstrap vcpkg into .vcpkg when needed."
    ON
)
if(ECHONEXUS_IS_TOP_LEVEL)
  set(ECHONEXUS_VCPKG_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
  set(ECHONEXUS_VCPKG_IS_TOP_LEVEL ON)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EchoNexusVcpkg.cmake")
endif()

project(EchoNexus VERSION 0.0.0 LANGUAGES CXX)

option(ECHONEXUS_DISABLE_BEAST "Disable Boost.Beast dependency" OFF)
option(ECHONEXUS_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(ECHONEXUS_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})
option(ECHONEXUS_BUILD_TESTS "Build tests" OFF)

# Find libraries
set(BOOST_COMPS asio system)

if(NOT ECHONEXUS_DISABLE_BEAST)
  list(APPEND BOOST_COMPS beast)
endif()

find_package(Boost REQUIRED CONFIG COMPONENTS ${BOOST_COMPS})
find_package(glaze REQUIRED CONFIG)

# Define EchoNexus library
add_library(EchoNexus)
add_library(EchoNexus::EchoNexus ALIAS EchoNexus)

file(GLOB_RECURSE ECHONEXUS_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

target_sources(EchoNexus
    PRIVATE
        ${ECHONEXUS_SOURCES}
)

target_include_directories(EchoNexus
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

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
    $<$<CXX_COMPILER_ID:GNU>:
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow=local
    >
    $<$<CXX_COMPILER_ID:Clang>:
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    >
    $<$<CXX_COMPILER_ID:AppleClang>:
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

include(GNUInstallDirs)

install(
    TARGETS EchoNexus
    EXPORT EchoNexusTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT EchoNexusTargets
    NAMESPACE EchoNexus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/EchoNexus
)

if(ECHONEXUS_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()

if(ECHONEXUS_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
