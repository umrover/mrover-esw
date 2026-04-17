cmake_minimum_required(VERSION 3.22)

file(GLOB INFLUXDB_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
)

if(INFLUXDB_SOURCES)
    add_library(influxdb STATIC ${INFLUXDB_SOURCES})
    set(influxdb_scope PUBLIC)
else()
    add_library(influxdb INTERFACE)
    set(influxdb_scope INTERFACE)
endif()

target_include_directories(influxdb ${influxdb_scope}
    "${CMAKE_CURRENT_SOURCE_DIR}/Inc"
)