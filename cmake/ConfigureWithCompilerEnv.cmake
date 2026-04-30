cmake_minimum_required(VERSION 3.25)

foreach(required_var ECHONEXUS_SOURCE_DIR ECHONEXUS_BINARY_DIR ECHONEXUS_BUILD_TYPE)
  if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
    message(FATAL_ERROR "${required_var} is required")
  endif()
endforeach()

set(configure_command
  "${CMAKE_COMMAND}"
  -S "${ECHONEXUS_SOURCE_DIR}"
  -B "${ECHONEXUS_BINARY_DIR}"
  -G Ninja
  "-DCMAKE_BUILD_TYPE=${ECHONEXUS_BUILD_TYPE}"
)

if(DEFINED ECHONEXUS_BUILD_TESTS AND NOT "${ECHONEXUS_BUILD_TESTS}" STREQUAL "")
  list(APPEND configure_command "-DECHONEXUS_BUILD_TESTS=${ECHONEXUS_BUILD_TESTS}")
endif()

if(DEFINED ECHONEXUS_BUILD_EXAMPLES AND NOT "${ECHONEXUS_BUILD_EXAMPLES}" STREQUAL "")
  list(APPEND configure_command "-DECHONEXUS_BUILD_EXAMPLES=${ECHONEXUS_BUILD_EXAMPLES}")
endif()

if(NOT DEFINED ECHONEXUS_COMPILER OR "${ECHONEXUS_COMPILER}" STREQUAL "")
  execute_process(
    COMMAND ${configure_command}
    COMMAND_ERROR_IS_FATAL ANY
  )
  return()
endif()

get_filename_component(cxx_basename "${ECHONEXUS_COMPILER}" NAME)
get_filename_component(cxx_directory "${ECHONEXUS_COMPILER}" DIRECTORY)
set(cxx_compiler "${ECHONEXUS_COMPILER}")

if(
  cxx_basename STREQUAL "cl"
  OR cxx_basename STREQUAL "cl.exe"
  OR cxx_basename STREQUAL "clang-cl"
  OR cxx_basename STREQUAL "clang-cl.exe"
)
  set(cc_compiler "${ECHONEXUS_COMPILER}")
elseif(cxx_basename STREQUAL "cc")
  set(cc_compiler "${ECHONEXUS_COMPILER}")
  set(cxx_basename "c++")
elseif(cxx_basename STREQUAL "cc.exe")
  set(cc_compiler "${ECHONEXUS_COMPILER}")
  set(cxx_basename "c++.exe")
elseif(cxx_basename STREQUAL "c++" OR cxx_basename STREQUAL "c++.exe")
  string(REGEX REPLACE "^c\\+\\+" "cc" cc_basename "${cxx_basename}")
elseif(cxx_basename MATCHES "^clang\\+\\+([-.0-9A-Za-z_]*)?(\\.exe)?$")
  string(REGEX REPLACE "^clang\\+\\+" "clang" cc_basename "${cxx_basename}")
elseif(cxx_basename MATCHES "^g\\+\\+([-.0-9A-Za-z_]*)?(\\.exe)?$")
  string(REGEX REPLACE "^g\\+\\+" "gcc" cc_basename "${cxx_basename}")
else()
  message(FATAL_ERROR "Unsupported --compiler value `${ECHONEXUS_COMPILER}`. Supported compiler names are `cc`, `c++`, `g++`, `clang++`, `cl`, and `clang-cl`.")
endif()

if(NOT DEFINED cc_compiler)
  if(cxx_directory STREQUAL "")
    set(cc_compiler "${cc_basename}")
  else()
    cmake_path(APPEND cxx_directory "${cc_basename}" OUTPUT_VARIABLE cc_compiler)
  endif()
endif()

if(cxx_compiler STREQUAL "${ECHONEXUS_COMPILER}")
  if(cxx_directory STREQUAL "")
    set(cxx_compiler "${cxx_basename}")
  else()
    cmake_path(APPEND cxx_directory "${cxx_basename}" OUTPUT_VARIABLE cxx_compiler)
  endif()
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "CC=${cc_compiler}" "CXX=${cxx_compiler}" ${configure_command}
  COMMAND_ERROR_IS_FATAL ANY
)
