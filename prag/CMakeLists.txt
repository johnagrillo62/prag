cmake_minimum_required(VERSION 3.16)

project(Astrei VERSION 0.1.0 LANGUAGES CXX)

message(STATUS "CMake version: ${CMAKE_VERSION}")
message(STATUS "Project version: ${PROJECT_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Put all built targets into /out folder for clarity
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out)

if(NOT MSVC)
    # Enable testing
    enable_testing()
endif()

include_directories(${CMAKE_SOURCE_DIR})

# Find packages

# Include main source directory
add_subdirectory(src)
add_subdirectory(tests)
