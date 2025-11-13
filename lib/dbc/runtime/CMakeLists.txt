cmake_minimum_required(VERSION 3.22)

project(dbc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(dbc STATIC
    src/file_parser.cpp
    src/frame_processor.cpp
    src/message.cpp
    src/signal.cpp
)
add_library(dbc::dbc ALIAS dbc)

target_include_directories(dbc
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

option(DBC_BUILD_TESTS "Build tests for the dbc library" OFF)
if(DBC_BUILD_TESTS)
  add_subdirectory(tests)
endif()

