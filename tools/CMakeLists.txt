cmake_minimum_required(VERSION 3.22)

project(tools)

find_program(UV_EXECUTABLE uv REQUIRED)

set(TOOLS_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "tools directory")
set(VENV_DIR ${TOOLS_DIR}/.venv)

if (WIN32)
    set(VENV_PYTHON ${VENV_DIR}/Scripts/python.exe CACHE INTERNAL "python installation")
else()
    set(VENV_PYTHON ${VENV_DIR}/bin/python CACHE INTERNAL "python installation")
endif()

set(PYPROJECT_TOML ${TOOLS_DIR}/pyproject.toml)
set(UV_LOCK ${TOOLS_DIR}/uv.lock)

# uv sync creates the venv itself and reads the interpreter from tools/.python-version,
# so there is no separate `uv venv` step to keep in sync with it.
add_custom_command(
    OUTPUT ${VENV_DIR}/package.timestamp
    DEPENDS ${PYPROJECT_TOML} ${UV_LOCK}
    WORKING_DIRECTORY ${TOOLS_DIR}
    COMMAND ${CMAKE_COMMAND} -E env UV_PROJECT_ENVIRONMENT=${VENV_DIR} ${UV_EXECUTABLE} sync --quiet
    COMMAND ${CMAKE_COMMAND} -E touch ${VENV_DIR}/package.timestamp
    COMMENT "installing esw python tools"
    VERBATIM
)

add_custom_target(python_package DEPENDS ${VENV_DIR}/package.timestamp)

add_custom_target(python_env_ready DEPENDS python_package)

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
