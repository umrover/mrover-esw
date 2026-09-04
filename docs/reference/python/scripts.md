# Script Reference

Every entry point in `tools/scripts/`. None are installed as commands; run them with `uv run` or
through a wrapper.

```bash
uv run --project tools python tools/scripts/<name>.py [flags]
```

## Summary

| Script | Invoked by | Purpose |
| --- | --- | --- |
| `generate_project.py` | `scripts/new.sh` | create a CubeMX project |
| `update_cmake_cfg.py` | `scripts/new.sh` | render a project's CMake files |
| `clangd.py` | `scripts/build.sh` | render a project's `.clangd` |
| `can_header_gen.py` | CMake (`lib/dbc`) | DBC to C++ headers |
| `config_gen.py` | CMake (`lib/config`) | config YAML to C++ header |
| `monitor.py` | `scripts/monitor.sh` | ST-LINK serial log viewer |
| `config.py` | by hand | push a board configuration over CAN |
| `analyze_flash.py` | by hand | flash usage breakdown from a `.map` file |
| `abs_zero.py` | by hand | zero the DE absolute encoders |
| `restart_bmc.py` | by hand | reset a brushed motor controller |
| `send_can.py` | by hand | bench loop driving a BMC |

## Build Pipeline

### `generate_project.py`

| Flag | Required | Meaning |
| --- | --- | --- |
| `--mcu`, `--board`, `-m`, `-b` | yes | MCU part number or board name |
| `--src`, `-s` | yes | destination directory; its name becomes the project name |

All four target flags write to the same value, so `--mcu` and `--board` are interchangeable
spellings. A value starting with `STM32` is treated as an MCU, anything else as a board.

### `update_cmake_cfg.py`

| Flag | Required | Meaning |
| --- | --- | --- |
| `--src`, `-s` | yes | project directory |
| `--root`, `-r` | yes | repository root |
| `--ctx`, `-c` | yes | template directory, always `lib/stm32g4` |
| `--lib`, `-l` | no | library to link; repeatable |

Rewrites `CMakeLists.txt`, `CMakePresets.json` and `.clangd` from scratch. Pass the full `--lib`
set every time. See [Project Anatomy](../build/project-layout.md#linked-libraries).

### `clangd.py`

| Flag | Required | Meaning |
| --- | --- | --- |
| `--src`, `-s` | yes | project directory |
| `--ctx`, `-c` | yes | template directory, always `lib/stm32g4` |

No-ops if `.clangd` already exists. Delete the file and re-run after a CubeCLT upgrade.

### `can_header_gen.py`

| Argument | Required | Meaning |
| --- | --- | --- |
| `files` | yes | one or more `.dbc` files (positional) |
| `--dest`, `-d` | yes | output directory |
| `--ctx`, `-c` | yes | template directory |

### `config_gen.py`

| Flag | Required | Default | Meaning |
| --- | --- | --- | --- |
| `--name`, `-n` | yes | | project name |
| `--input`, `-i` | yes | | config definition YAML |
| `--output`, `-o` | yes | | header to write |
| `--tabsize` | no | `4` | accepted but unused |
| `--template-dir` | no | cwd | directory holding `config_header.hpp.j2` |

## Operational Tools

### `monitor.py`

| Flag | Default | Meaning |
| --- | --- | --- |
| `--baud`, `-b` | `115200` | must match the firmware UART |
| `--log-level`, `-l` | `INFO` | `DEBUG`, `INFO`, `WARNING` or `ERROR` |

See [esw.stlink](stlink.md).

### `config.py`

| Flag | Required | Meaning |
| --- | --- | --- |
| `--definition`, `-d` | yes | register layout from `config/` |
| `--file`, `-f` | yes | device values from `rover/` |
| `--can`, `-c` | yes | CAN interface, e.g. `can0` |
| `--id`, `-i` | yes | the board's current CAN ID |
| `--read`, `-r` | no | **not implemented** |

See [CAN Configuration Interface](../config/can-interface.md).

### `analyze_flash.py`

```bash
uv run --project tools python tools/scripts/analyze_flash.py src/bmc/build/Debug/bmc.map
```

Takes a linker map file as its only positional argument, with no flags. Sums the flash region by
object file and prints the 25 largest contributors plus a total. Useful when a build starts
crowding the part's flash. The `.map` is produced by every build, next to the ELF.

This is the one script that does not import `esw`.

## Bench Scripts

!!! warning
    These have **hardcoded** CAN IDs and interface names and take no arguments. Read the
    source before running any of them on real hardware.

| Script | Interface | Targets | Sends |
| --- | --- | --- | --- |
| `abs_zero.py` | `can1` | `0x37` pitch, `0x38` roll | `ABSZeroCmd` with `offset: 0.0`, then listens 10 s |
