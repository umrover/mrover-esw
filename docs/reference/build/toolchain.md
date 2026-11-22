# Toolchain and Presets

Both the toolchain file and the preset file are **per-project** artifacts that arrive from CubeMX.
They are not shared, though in practice every project carries the same content.

## `CMakePresets.json`

Two usable presets, both inheriting a hidden `default`:

```json
{
  "name": "default", "hidden": true,
  "generator": "Ninja",
  "binaryDir": "${sourceDir}/build/${presetName}",
  "toolchainFile": "${sourceDir}/cmake/gcc-arm-none-eabi.cmake",
  "cacheVariables": { "CMAKE_EXPORT_COMPILE_COMMANDS": "ON" }
}
```

| Preset | `CMAKE_BUILD_TYPE` | Optimization | Use it for |
| --- | --- | --- | --- |
| `Debug` | `Debug` | `-O0 -g3` | anything you intend to step through |
| `Release` | `RelWithDebInfo` | `-Os -g0` | flight builds |

!!! note
    `Release` sets `CMAKE_BUILD_TYPE=RelWithDebInfo`, not `Release`, so a generator expression
    written as `$<$<CONFIG:Release>:...>` never fires. The project template accounts for this by
    matching both configurations:

    ```cmake
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        STM32
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release,RelWithDebInfo>:RELEASE>
    )
    ```

    Keep that in mind when adding your own per-configuration settings. Note that nothing in the
    firmware currently guards on `DEBUG` or `RELEASE`; both are provided for you to use.

Build output always lands at:

```
<project>/build/<preset>/<target>.elf     e.g.  src/bmc/build/Debug/bmc.elf
```

## The Toolchain File

`cmake/gcc-arm-none-eabi.cmake` is what every preset points at. The parts worth knowing:

| Setting | Value |
| --- | --- |
| System | `CMAKE_SYSTEM_NAME Generic`, processor `arm` |
| Compilers | `arm-none-eabi-gcc`, `-g++`, `-objcopy`, `-size` |
| CPU flags | `-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard` |
| C++ flags | C flags plus `-fno-rtti -fno-exceptions -fno-threadsafe-statics` |
| Linker | `-T ${CMAKE_SOURCE_DIR}/STM32G431XX_FLASH.ld`, `--specs=nano.specs`, `--gc-sections` |
| Try-compile | `STATIC_LIBRARY` (a freestanding target cannot link a test executable) |

Two consequences worth remembering:

- **The compiler is resolved from `PATH`.** The toolchain names the tools bare, so whichever
  `arm-none-eabi-gcc` comes first wins. `/etc/profile.d/mrover-esw.sh` puts CubeCLT ahead of the
  standalone fallback; `./scripts/doctor.sh` warns when that ordering has gone stale.
- **The linker script path is fixed** relative to the project root. Every project must keep its
  `STM32G431XX_FLASH.ld` at its own top level.

## `starm-clang.cmake`

Every project also ships a toolchain file for ST's LLVM distribution. No preset references it and
`build.sh` has no way to select it, so it is inert. Using it means invoking CMake by hand with
`-DCMAKE_TOOLCHAIN_FILE=`.

