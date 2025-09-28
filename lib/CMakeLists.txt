cmake_minimum_required(VERSION 3.22)

project(fwlib LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# header-only
add_library(fwlib INTERFACE)

target_include_directories(fwlib INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)
