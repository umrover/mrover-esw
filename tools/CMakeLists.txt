cmake_minimum_required(VERSION 3.22)

project(tools)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "tools directory")
set(VENV_DIR ${TOOLS_DIR}/venv)

if (WIN32)
    set(VENV_PYTHON ${VENV_DIR}/Scripts/python.exe CACHE INTERNAL "python installation")
    set(VENV_PIP ${VENV_DIR}/Scripts/pip.exe)
else()
    set(VENV_PYTHON ${VENV_DIR}/bin/python CACHE INTERNAL "python installation")
    set(VENV_PIP ${VENV_DIR}/bin/pip)
endif()

set(PYPROJECT_TOML ${TOOLS_DIR}/pyproject.toml)
set(SETUP_SCRIPT ${TOOLS_DIR}/setup_venv.py)

# auto reconfigure on pyproject change
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${PYPROJECT_TOML})

message(STATUS "executing venv setup")
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${SETUP_SCRIPT} ${VENV_DIR} ${PYPROJECT_TOML} ${VENV_PIP}
    RESULT_VARIABLE VENV_RESULT
    OUTPUT_VARIABLE VENV_OUTPUT
    ERROR_VARIABLE VENV_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT VENV_RESULT EQUAL 0)
    message(FATAL_ERROR "python environment setup failed\n${VENV_OUTPUT}\n${VENV_ERROR}")
elseif(VENV_OUTPUT MATCHES "^WARNING:")
    string(REGEX REPLACE "^WARNING:[ \t]*" "" CLEAN_WARNING "${VENV_OUTPUT}")
    message(WARNING "${CLEAN_WARNING}")
elseif(VENV_OUTPUT MATCHES "^STATUS:")
    string(REGEX REPLACE "^STATUS:[ \t]*" "" CLEAN_STATUS "${VENV_OUTPUT}")
    message(STATUS "${CLEAN_STATUS}")
endif()

add_custom_target(python_env_ready)

function(run_python_tool TARGET_NAME SCRIPT)
    add_custom_target(
        ${TARGET_NAME}
        COMMAND ${VENV_PYTHON} ${SCRIPT} ${ARGN}
        DEPENDS python_env_ready
        WORKING_DIRECTORY ${TOOLS_DIR}
        COMMENT "running tool ${SCRIPT}"
        VERBATIM
    )
endfunction()

add_custom_target(clean_venv
    COMMAND ${CMAKE_COMMAND} -E rm -rf ${VENV_DIR}
    COMMENT "removing venv"
)
