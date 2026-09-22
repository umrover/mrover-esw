cmake_minimum_required(VERSION 3.22)

file(GLOB YAML_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

if(YAML_SOURCES)
    add_library(yaml STATIC ${YAML_SOURCES})
    set(yaml_scope PUBLIC)
else()
    add_library(yaml INTERFACE)
    set(yaml_scope INTERFACE)
endif()

target_include_directories(yaml ${yaml_scope}
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)
