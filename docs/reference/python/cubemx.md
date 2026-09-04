# `esw.cubemx`

Drives STM32CubeMX headlessly and renders the CMake files for a generated project. This is what
`scripts/new.sh` is built on.

## `esw.cubemx`

```python
get_installation()
run_cubemx_script(script_content: str, return_output_on_failure: bool = False)
```

`get_installation` resolves `STM32CubeMX` on `PATH`, raising `RuntimeError` if absent. Cached.

`run_cubemx_script` writes a script to a temporary file and runs `STM32CubeMX -q <file>`. CubeMX
reports failure by printing `KO` rather than by exit code, so both are checked. Returns a bool, or
`(bool, output)` when `return_output_on_failure` is set.

## `esw.cubemx.generation`

```python
generate_project(name: str, path: Path, mcu: str) -> None
```

Creates a CubeMX project in two passes.

The first pass generates the project with a CMake toolchain and GCC:

```
load STM32G431CBTx            # or: loadboard NUCLEO-G431RB nomode
project name <name>
project path <path>
project toolchain CMake
project compiler GCC
SetCopyLibrary copy as reference
project generate
exit
```

A target starting with `STM32` is loaded as an MCU; anything else (`NUCLEO-*`, `DISCO-*`) is
loaded as a board.

The second pass patches the generated `.ioc`, replacing `LibraryCopy=0` with `LibraryCopy=2`
("copy as reference"), deletes the vendored `Drivers/` directory, and regenerates. That is what
stops CubeMX from copying a private copy of the HAL into every project. This repo builds the HAL
once from the `STM32CubeG4` submodule instead.

## `esw.cubemx.cmake`

```python
configure_cmake(name: str, path: Path, root: Path, ctx: Path, libs: list[str]) -> None
configure_clang(path: Path, ctx: Path) -> None
```

### `configure_cmake`

Deletes and re-renders a project's build files. In order:

1. Removes `.clangd`, `CMakeLists.txt`, `CMakePresets.json` and `cmake/stm32cubemx/CMakeLists.txt`.
2. Discovers the source directory (`Src/` or `Core/Src/`), the include directory (`Inc/` or
   `Core/Inc/`), and the startup assembly file. Exactly one `*.s` must exist at the project root,
   otherwise it raises.
3. Computes the relative path from the project to `lib/`, which is what makes
   `add_subdirectory(../../lib fwlib)` come out right at any nesting depth.
4. Renders three templates from `ctx` (always `lib/stm32g4`): `CMakeLists.txt.j2`,
   `CMakePresets.json.j2` and `.clangd.j2`.

!!! warning
    This is a full rewrite, not a patch. Anything hand-edited in those three files is lost, and
    the `libs` list is not recovered from the existing file, so pass the complete `--lib` set every
    time. See [Project Anatomy](../build/project-layout.md#linked-libraries).

The toolchain files under `cmake/` are **not** touched; they come from CubeMX and stay as they are.

### `configure_clang`

Renders `.clangd` only, and no-ops if the file already exists. `scripts/build.sh` calls this on
every build so a fresh checkout gets a working clangd configuration without a separate step.

It locates the newest CubeCLT install by globbing `/opt/st/stm32cubeclt_*` and picking by
modification time rather than by name, because name sorting puts `1.9.0` above `1.20.0`. It needs
three include paths and warns if it cannot find all three.

## Usage

Normally through the wrapper:

```bash
./scripts/new.sh --board NUCLEO-G431RB --src src/example --lib stm32 --lib dbc
```

Directly, to re-render an existing project:

```bash
uv run --project tools python tools/scripts/update_cmake_cfg.py \
    --src src/example --root . --ctx lib/stm32g4 --lib stm32 --lib dbc
```
