cmake_minimum_required(VERSION 3.22)

project(fwlib LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(dbc)
add_subdirectory(stm32g4)
add_subdirectory(stm32)
add_subdirectory(units)
add_subdirectory(util)
