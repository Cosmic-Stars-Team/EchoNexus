cmake_minimum_required(VERSION 3.25)

# Get vcpkg toolchain setup if not already provided
set(ECHONEXUS_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ECHONEXUS_IS_TOP_LEVEL ON)
endif()

option(
    ECHONEXUS_AUTO_SETUP_VCPKG
    "Auto clone/bootstrap vcpkg into .vcpkg when needed."
    ON
)

set(ECHONEXUS_LOCAL_VCPKG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.vcpkg")
set(
    ECHONEXUS_LOCAL_VCPKG_TOOLCHAIN
    "${ECHONEXUS_LOCAL_VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake"
)
set(
    ECHONEXUS_ENV_VCPKG_TOOLCHAIN
    "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
)

if(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_TOOLCHAIN_FILE}")
    message(
        STATUS
        "Using explicit CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}"
    )
elseif(EXISTS "${ECHONEXUS_ENV_VCPKG_TOOLCHAIN}")
    set(
        CMAKE_TOOLCHAIN_FILE
        "${ECHONEXUS_ENV_VCPKG_TOOLCHAIN}"
        CACHE FILEPATH "vcpkg toolchain file" FORCE
    )
    message(STATUS "Using vcpkg from VCPKG_ROOT: ${CMAKE_TOOLCHAIN_FILE}")
elseif(ECHONEXUS_IS_TOP_LEVEL AND ECHONEXUS_AUTO_SETUP_VCPKG)
    find_program(ECHONEXUS_GIT_EXECUTABLE git)
    if(NOT ECHONEXUS_GIT_EXECUTABLE)
        message(
            FATAL_ERROR
            "git is required to auto-setup vcpkg. "
            "Install git or provide CMAKE_TOOLCHAIN_FILE."
        )
    endif()

    if(NOT EXISTS "${ECHONEXUS_LOCAL_VCPKG_DIR}/.git")
        message(STATUS "Cloning vcpkg to ${ECHONEXUS_LOCAL_VCPKG_DIR}")
        execute_process(
            COMMAND "${ECHONEXUS_GIT_EXECUTABLE}"
                clone
                https://github.com/microsoft/vcpkg.git
                "${ECHONEXUS_LOCAL_VCPKG_DIR}"
            RESULT_VARIABLE ECHONEXUS_VCPKG_CLONE_RESULT
        )
        if(NOT ECHONEXUS_VCPKG_CLONE_RESULT EQUAL 0)
            message(
                FATAL_ERROR
                "Failed to clone vcpkg "
                "(exit code: ${ECHONEXUS_VCPKG_CLONE_RESULT})."
            )
        endif()
    endif()

    message(STATUS "Bootstrapping local vcpkg in ${ECHONEXUS_LOCAL_VCPKG_DIR}")
    if(WIN32)
        execute_process(
            COMMAND cmd /c
                "${ECHONEXUS_LOCAL_VCPKG_DIR}/bootstrap-vcpkg.bat"
                -disableMetrics
            WORKING_DIRECTORY "${ECHONEXUS_LOCAL_VCPKG_DIR}"
            RESULT_VARIABLE ECHONEXUS_VCPKG_BOOTSTRAP_RESULT
        )
    else()
        execute_process(
            COMMAND "${ECHONEXUS_LOCAL_VCPKG_DIR}/bootstrap-vcpkg.sh"
                -disableMetrics
            WORKING_DIRECTORY "${ECHONEXUS_LOCAL_VCPKG_DIR}"
            RESULT_VARIABLE ECHONEXUS_VCPKG_BOOTSTRAP_RESULT
        )
    endif()

    if(NOT ECHONEXUS_VCPKG_BOOTSTRAP_RESULT EQUAL 0)
        message(
            FATAL_ERROR
            "Failed to bootstrap vcpkg "
            "(exit code: ${ECHONEXUS_VCPKG_BOOTSTRAP_RESULT})."
        )
    endif()

    if(NOT EXISTS "${ECHONEXUS_LOCAL_VCPKG_TOOLCHAIN}")
        message(
            FATAL_ERROR
            "vcpkg toolchain not found after bootstrap: "
            "${ECHONEXUS_LOCAL_VCPKG_TOOLCHAIN}"
        )
    endif()

    set(
        CMAKE_TOOLCHAIN_FILE
        "${ECHONEXUS_LOCAL_VCPKG_TOOLCHAIN}"
        CACHE FILEPATH "vcpkg toolchain file" FORCE
    )
    message(STATUS "Using local vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
elseif(ECHONEXUS_IS_TOP_LEVEL)
    message(FATAL_ERROR
        "No valid vcpkg toolchain file found.\n"
        "Set CMAKE_TOOLCHAIN_FILE, set VCPKG_ROOT, "
        "or keep ECHONEXUS_AUTO_SETUP_VCPKG=ON."
    )
endif()

project(EchoNexus VERSION 0.0.0 LANGUAGES CXX)

option(ECHONEXUS_DISABLE_BEAST "Disable Boost.Beast dependency" OFF)
option(ECHONEXUS_WARNINGS_AS_ERRORS "Treat warnings as errors" ON)
option(ECHONEXUS_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

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
        -Wshadow
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
