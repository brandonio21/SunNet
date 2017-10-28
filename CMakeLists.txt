cmake_minimum_required(VERSION 2.6)

project(SunNet)

file(GLOB SOURCES src/*.cpp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(SunNet STATIC ${SOURCES})

add_subdirectory(examples)
