cmake_minimum_required(VERSION 3.14)

project(mipt-cpp)

set(CMAKE_CXX_STANDARD             20)
set(CMAKE_MODULE_PATH              "${CMAKE_SOURCE_DIR}/tools/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_EXPORT_COMPILE_COMMANDS  ON)

find_package(Catch REQUIRED)

include(tools/cmake/TestSolution.cmake)
include(tools/cmake/BuildFlags.cmake)
include_directories(tools/util)

function(add_if_exists name)
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name})
    add_subdirectory(${name})
  endif()
endfunction()

add_if_exists(private)

add_if_exists(multiplication)

add_if_exists(iostream)

add_if_exists(recursive_directory_iterator)

add_if_exists(malloc)
