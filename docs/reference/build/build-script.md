# Build Script Internals

`scripts/build.sh` is the entry point for every build, local and CI. This page covers what it does
under the hood; for everyday usage see [Build Tools](../../info/build.md#buildsh).

## Flags

| Flag | Default | Meaning |
| --- | --- | --- |
| `-s`, `--src` | required | project directory |
| `-p`, `--preset` | `Debug` | `Debug` or `Release` |
| `-t`, `--target` | basename of `--src` | CMake target, if it differs from the directory name |
| `-f`, `--flash` | off | flash the ELF after a successful build |
| `-c`, `--clean` | off | remove build artifacts and exit |
| `-h`, `--help` | | usage |

Three environment variables tune flashing:

| Variable | Default | Passed to |
| --- | --- | --- |
| `PORT` | `swd` | `STM32_Programmer_CLI --connect port=` |
| `FREQ` | `8000` | `freq=` |
| `RESET` | `HWrst` | `reset=` |

## Sequence

1. **Dependency check.** `cmake`, `ninja` and `uv` must be on `PATH`; `STM32_Programmer_CLI` is
   required only with `--flash`.
2. **Submodule check.** `lib/stm32g4/STM32CubeG4/.git` must exist, otherwise it prints the
   `git submodule update --init --recursive` hint and exits.
3. **Clean, if asked.** `--clean` removes three things and **exits without building**:
   the project's `build/<preset>` directory, `tools/.venv`, and the shared `lib/dbc/build`.
4. **Configure, only if needed.** CMake runs only when `build/<preset>/build.ninja` is absent:

    ```bash
    cmake --preset "$PRESET"
    ```

    No `-D` flags are passed; the preset supplies the generator, build directory, toolchain file
    and build type. Ninja re-runs CMake itself when a `CMakeLists.txt` changes, so skipping is
    safe. A genuinely stale cache is what `--clean` is for.

5. **Build.**

    ```bash
    cmake --build --target "$TARGET_NAME" --preset "$PRESET"
    ```

    Parallelism comes from `CMAKE_BUILD_PARALLEL_LEVEL` in the environment; the script sets no
    `-j` of its own.

6. **Flash, if asked.**

    ```bash
    STM32_Programmer_CLI --connect port=$PORT freq=$FREQ reset=$RESET \
        --write <elf> --verify --start
    ```

7. **`.clangd`.** If the project has no `.clangd`, one is generated:

    ```bash
    uv run --quiet --project tools python tools/scripts/clangd.py \
        --src "$SRC_DIR" --ctx lib/stm32g4
    ```

    This uses `uv run` rather than the venv Python directly so it works before any venv exists.
    The file is gitignored and contains machine-specific include paths, so it is per-developer.

## Why the Target Name Matters

The build is target-scoped, so `--target` must match `CMAKE_PROJECT_NAME` in the generated
`CMakeLists.txt`. For projects created by `scripts/new.sh` the directory name and the target name
are the same and you can omit the flag.
