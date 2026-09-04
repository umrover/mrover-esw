cmake_minimum_required(VERSION 3.22)

file(GLOB_RECURSE RTOS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

if(MROVER_USE_RTOS)
    add_library(fwrtos OBJECT ${RTOS_SOURCES})
    set(rtos_scope PUBLIC)
    target_compile_definitions(fwrtos PRIVATE STM32)
else()
    add_library(fwrtos INTERFACE)
    set(rtos_scope INTERFACE)
endif()

target_include_directories(fwrtos ${rtos_scope}
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)

target_link_libraries(fwrtos ${rtos_scope}
    stm32cubemx
    stm32 # hooks.cpp routes stack overflows into mrover::System::fault()
)
