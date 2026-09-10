# Build System Overview

Firmware is built with CMake and the ARM GNU toolchain from STM32CubeCLT. STM32CubeMX generates
the hardware initialization code, but never builds anything. `scripts/build.sh` is the only
entry point you normally need.

## Workflow

Each project under `src/` is a **standalone CMake project root**. It has its own
`CMakeLists.txt`, `CMakePresets.json` and `cmake/` toolchain files, and it pulls in the shared
firmware library tree with a single line:

```cmake
add_subdirectory(../../lib fwlib)
```

`lib/` then builds the shared libraries and, critically, defines the `stm32cubemx` target that
CubeMX would otherwise have generated separately inside every project. One shared definition
replaces thirteen copies.

## What Happens on a Build

Running `./scripts/build.sh --src src/bmc --preset Debug` sets off the following chain:

1. **`build.sh` runs `cmake --preset Debug`.** No `-D` flags are passed, since the preset carries
   everything CMake needs.
2. **`CMakePresets.json` selects the build.** It picks the Ninja generator, the `build/Debug`
   output directory, and the toolchain file.
3. **`cmake/gcc-arm-none-eabi.cmake` selects the compiler.** It resolves `arm-none-eabi-gcc` from
   `PATH`, applies the Cortex-M4 flags, and points the linker at `STM32G431XX_FLASH.ld`.
4. **`src/bmc/CMakeLists.txt` declares the project.** It sets `MX_SRC_DIR`, `MX_INC_DIR` and
   `MX_STARTUP_S`, then pulls in the shared tree with `add_subdirectory(../../lib fwlib)`.
5. **`lib/CMakeLists.txt` builds the libraries.** In order:

    - `tools/` creates the `uv` virtual environment and provides the `python_env_ready` target
      that both code generators depend on.
    - `lib/dbc/` generates `MRoverCAN.hpp` from `dbc/MRoverCAN.dbc`.
    - `lib/config/` generates `<project>_config.hpp` from `config/<project>.yaml`.
    - `lib/stm32g4/` defines `stm32cubemx` and `STM32_Drivers` over the vendored CubeG4 submodule.
    - `lib/stm32/` and `lib/util/` provide the hand-written driver and utility headers.

6. **The linker produces `src/bmc/build/Debug/bmc.elf`**, alongside a `.map` file you can inspect
   with `analyze_flash.py`.

Steps 2 and 3 happen only on the first build. Afterwards Ninja re-runs CMake itself when a
`CMakeLists.txt` changes, so subsequent builds start at step 5. See
[Build Script Internals](build-script.md).

## The Library Tree

| Target | Source | What it gives you |
| --- | --- | --- |
| `stm32cubemx` | `lib/stm32g4/CMakeLists.txt` | HAL include paths and `-D` defines |
| `STM32_Drivers` | `lib/stm32g4/CMakeLists.txt` | the compiled STM32G4 HAL |
| `stm32` | `lib/stm32/` | board hardware drivers (`hw/`, `serial/`, `adc.hpp`, `timer.hpp`) |
| `util` | `lib/util/` | `logger.hpp`, `pidf.hpp`, `filtering.hpp`, `util.hpp` |
| `dbc` | generated | CAN message classes from `dbc/MRoverCAN.dbc` |
| `config` | generated | the board's register struct from `config/<project>.yaml` |

A project selects which of these it links through `--lib` flags on `scripts/new.sh`. See
[Project Anatomy](project-layout.md).

!!! note
    `lib/stm32` and `lib/util` link each other. That cycle is legal only because both currently
    resolve to `INTERFACE` libraries (neither has any `.cpp` files yet). Adding a source file to
    either one will break the configure step until the cycle is removed.

## Where to Go Next

- [Project Anatomy](project-layout.md): what is in a project directory, and what you may edit
- [Toolchain and Presets](toolchain.md): compiler selection, `Debug` vs `Release`
- [Generated Libraries](codegen.md): how the DBC and config headers get built
- [Build Script Internals](build-script.md): what `build.sh` actually runs
- [Continuous Integration](ci.md): what CI builds, and in what container
