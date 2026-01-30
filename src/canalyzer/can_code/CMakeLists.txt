cmake_minimum_required(VERSION 3.22)
set(CMAKE_CXX_STANDARD 23)

project(canalyzer LANGUAGES CXX)

add_compile_options(
    $<$<CONFIG:Debug>:-g3>
)
#add_compile_options(-Wall -Werror)

get_filename_component(DBC_RUNTIME_ROOT 
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/dbc/runtime" 
    ABSOLUTE
)

add_subdirectory("../../../lib/dbc/runtime"
                dbc_runtime_build)

add_executable(canalyzer
    src/driver.cpp
    src/logger.cpp
    src/Yaml.cpp
)

target_link_libraries(canalyzer PUBLIC dbc_runtime)

target_include_directories(canalyzer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)
