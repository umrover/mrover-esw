cmake_minimum_required(VERSION 3.22)

file(GLOB UNITS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

if(STM32_SOURCES)
    add_library(linux STATIC ${UNITS_SOURCES})
else()
    add_library(linux INTERFACE)
endif()

target_include_directories(linux INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)
