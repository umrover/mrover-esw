cmake_minimum_required(VERSION 3.22)

file(GLOB HW_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

if(HW_SOURCES)
    add_library(hw STATIC ${HW_SOURCES})
    set(hw_scope PUBLIC)
else()
    add_library(hw INTERFACE)
    set(hw_scope INTERFACE)
endif()

target_include_directories(hw ${hw_scope}
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)

target_link_libraries(hw ${hw_scope}
    stm32cubemx
    stm32
    units
    util
)
