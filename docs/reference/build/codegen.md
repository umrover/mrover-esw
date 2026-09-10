# Generated Libraries

Two libraries are generated from source files at build time, and a third wraps the vendored ST
HAL. All three follow the same pattern.

## The Pattern

```cmake
add_custom_command(
    OUTPUT  ${GENERATED_HEADER}
    COMMAND ${VENV_PYTHON} ${SCRIPT} --input ... --output ...
    DEPENDS ${INPUT_FILE} ${SCRIPT} ${TEMPLATE} python_env_ready
    WORKING_DIRECTORY ${TOOLS_DIR}
    VERBATIM)

add_library(<name> INTERFACE ${GENERATED_HEADER})
target_include_directories(<name> INTERFACE ${GEN_DIR})
```

A Python script renders a Jinja2 template into a C++ header, and an `INTERFACE` library carries
that header plus its include directory.

The `python_env_ready` dependency comes from `tools/CMakeLists.txt`, which runs `uv sync` into
`tools/.venv` whenever `pyproject.toml` or `uv.lock` changes. This is why the first build of any
project transparently creates the virtual environment.

## CAN Message Classes (`lib/dbc`)

| Type | Function |
| --- | --- |
| Input | `dbc/*.dbc` (currently just `MRoverCAN.dbc`) |
| Script | `tools/scripts/can_header_gen.py` |
| Template | `lib/dbc/templates/dbc_header.hpp.j2` |
| Output | `lib/dbc/build/MRoverCAN.hpp` |
| Target | `dbc` |

Produces one class per CAN message with a `CAN_ID`, typed signal members, and constructors that
pack and unpack the byte array. It also defines the ID masks shared with the Python side
(`CAN_DEST_ID_MASK`, `CAN_SRC_ID_MASK`, and the moteus prefix constants).

!!! warning
    The output directory `lib/dbc/build/` is **inside the source tree and shared by every
    project**, not per build directory. Building two projects concurrently races on it.
    `./scripts/build.sh --clean` deletes it.

To hand-edit a generated header for debugging, comment out the `add_custom_command` block in
`lib/dbc/CMakeLists.txt` so it stops being regenerated.

## Board Register Struct (`lib/config`)

| Type | File |
| --- | --- |
| Input | `config/<project>.yaml` |
| Script | `tools/scripts/config_gen.py` |
| Template | `lib/config/templates/config_header.hpp.j2` |
| Output | `<MX_INC_DIR>/<project>_config.hpp` |
| Target | `config` |

The YAML is matched to the project purely by filename: `src/bmc` gets `config/bmc.yaml`. A project
with no matching YAML logs a warning and skips the target entirely, which is why only `abs`, `bmc`
and `lim` have one today.

Output goes into the project's own `Inc/` directory and is gitignored as `**/*_config.hpp`.

See [Register Definitions](../config/schema.md) for the YAML schema and the generated API.

!!! note
    A board only gets this header if it lists `config` in its `target_link_libraries`. Projects that do not list this would not build the header even if a matching YAML
    appeared. When adding a config to a board, pass `--lib config` when regenerating so the target
    is actually linked.

## The HAL (`lib/stm32g4`)

Not codegen, but the same kind of substitution. CubeMX normally emits
`<project>/cmake/stm32cubemx/CMakeLists.txt` defining a `stm32cubemx` target. This repo deletes
that file during generation, gitignores it so a CubeMX re-run cannot reintroduce it, and defines
the target once in `lib/stm32g4/CMakeLists.txt` instead.

That shared definition globs the entire STM32G4 HAL out of the `STM32CubeG4` submodule rather than
listing files per project:

```cmake
file(GLOB_RECURSE DRIVER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/STM32CubeG4/Drivers/STM32G4xx_HAL_Driver/Src/*.c")
list(FILTER DRIVER_SOURCES EXCLUDE REGEX ".*_template\\.c$")
```

Every project compiles the whole HAL and `--gc-sections` strips what it does not use. That trades
some build time for never having to re-sync a driver list after an `.ioc` change.

The submodule must be initialized or `build.sh` refuses to run:

```bash
git submodule update --init --recursive
```

## Other Libraries

`lib/stm32` and `lib/util` are ordinary header libraries, no generation involved.

- `lib/stm32/Inc/`: Defines abstractions of commonly-used functionality.
- `lib/util/Inc/`: Contains a collection utility functions and classes.

Both currently have empty `Src/` directories, so both resolve to `INTERFACE` libraries.
