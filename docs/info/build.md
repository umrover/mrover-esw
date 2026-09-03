# Build Tools

This document covers the build tooling and other utilities developed to aid the
development process.

## Scripts

All of the scripts found in `scripts/` serve as wrappers of either python tools
or vendor utilities (e.g. MJBots, STMicroelectronics), and should be used as
simple interfaces to these tools.

### `bootstrap.sh`

The bootstrap script sets up a fresh Ubuntu/Debian machine for ESW development: system
packages, the ARM GNU toolchain, `uv`, STM32CubeMX/CubeProgrammer/CubeCLT (and optionally
CubeIDE), git submodules, PATH setup, desktop launcher entries, VS Code and its extensions, and
the Python venv, all via an Ansible playbook
(`ansible/bootstrap.yml`). It installs Ansible itself if it isn't already present. Run it once on
a new machine as follows.

```bash
./scripts/bootstrap.sh
```

STM32CubeMX, STM32CubeProgrammer and STM32CubeCLT require a login-gated manual download from ST's
website, so the script will pause partway through and ask you to download all three installers
into the gitignored `install/` directory before continuing; see
[STM32Cube\*](../getting-started/stm32cube/index.md) for details.

CubeCLT ships `STM32_Programmer_CLI`, which is what the build scripts and CI use, but not the
programmer GUI. The full STM32CubeProgrammer is installed separately for interactive flashing,
option-byte editing and reading memory back off a board. It is deliberately **not** added to
`PATH` — its `bin/` contains a second `STM32_Programmer_CLI` that would shadow CubeCLT's — so it
is launched from its desktop entry instead.

Because CubeMX and CubeProgrammer ship graphical installers with no silent-install flag, two
installer windows open during the run for you to click through. Each installer is recorded against
a stamp file in `install/`, keyed to the installer's own filename, so re-running `bootstrap.sh`
does not pop those windows again, while dropping a **newer** installer into `install/` does
re-run it. That is how you upgrade: download the new archives, re-run `./scripts/bootstrap.sh`.

The IzPack installers write their own `.desktop` files, but they run under `sudo`, so those land
in root's home and never reach your launcher. `ansible/tasks/desktop-entries.yml` writes managed
entries for STM32CubeMX, STM32CubeProgrammer and STM32CubeIDE into `~/.local/share/applications`
instead.

STM32CubeIDE is the one optional piece: if its archive isn't in `install/`, the playbook says so
and moves on. It is used purely as a graphical debugger against an ELF `build.sh` produced, never
to build — see
[Debugging with STM32CubeIDE](../getting-started/stm32cube/index.md#debugging-with-stm32cubeide).

The ARM cross-compiler comes from STM32CubeCLT, which bundles the same `14.3.rel1` release ESW
builds with. The standalone ARM GNU toolchain is also installed, but only as a fallback: the
`PATH` written to `/etc/profile.d/mrover-esw.sh` puts CubeCLT's copy first and appends the
standalone one, so CubeCLT wins whenever it is present. `cmake` also comes from CubeCLT, which
ships 4.3.1 where the newest apt has is 4.2.3; `ninja` stays on apt's copy, which is the same
version either way.

Toolchain versions are pinned in three places that must move together when CubeCLT is upgraded:
`arm_gnu_link`/`arm_gnu_name` in `ansible/bootstrap.yml`, `ARM_GNU_LINK` in `Dockerfile.arm-gnu`,
and `EXPECTED_GCC_VERSION` in `scripts/doctor.sh`. Dependabot cannot see any of them — it does not
read apt package versions, ST's login-gated downloads, or a toolchain URL pinned in an `ENV`.
`doctor.sh` is the backstop: it warns when the compiler actually in use drifts from what CI
builds with.

`uv` is the exception, and is pinned in exactly one place:

```dockerfile
FROM ghcr.io/astral-sh/uv:0.12.8 AS uv
```

Everything reads that one line. CI and the release workflow run inside the image and get the
binary it copies out; `.github/workflows/site.yml` greps the tag and hands it to `setup-uv`;
`ansible/tasks/uv.yml` greps it too and uses `uv self update <version>`, which converges from
either direction, so a developer's `uv` matches CI's exactly. It has to be a named `FROM` stage
rather than an inline `COPY --from=ghcr.io/astral-sh/uv:…` because Dependabot's Docker ecosystem
parses `FROM` lines only — that is what makes this pin update itself.

Keeping it identical everywhere matters because `uv` writes `tools/uv.lock`, and the lockfile
revision moves with the tool. `required-version` in `tools/pyproject.toml` is a matching floor, so
an older `uv` fails loudly instead of silently rewriting the lock at a revision CI cannot read.

Once bootstrap finishes, open a new terminal and run [`doctor.sh`](#doctorsh) to confirm the
install is good.

#### Editor

VS Code is installed alongside the toolchain, with the extensions this repo's workflow needs.
The list is **not** kept in the playbook — it lives in `.vscode/extensions.json`, the file VS Code
already reads to prompt contributors, so the prompt and the playbook cannot drift apart:

| Extension | Why |
| --- | --- |
| `stmicroelectronics.stm32-vscode-extension` | ST's own tooling |
| `llvm-vs-code-extensions.vscode-clangd` | `build.sh` generates a `.clangd` per project |
| `marus25.cortex-debug` | drives CubeCLT's `ST-LINK_gdbserver`, the non-CubeIDE debug path |

`ms-vscode.cpptools` is listed under `unwantedRecommendations` on purpose: it and clangd both
provide IntelliSense, and running the two together produces duplicated and conflicting diagnostics.

Only missing extensions are installed — VS Code keeps them up to date itself. VS Code proper is
updated by `apt`/`brew` like any other package, which is why the playbook writes an **enabled**
`/etc/apt/sources.list.d/vscode.sources`: the `code` package ships that file disabled in some
installs, which silently orphans the editor at whatever version was first installed.

Skip the editor entirely with `--skip-tags vscode` if you use CLion or something else; nothing in
the build depends on it.

### `doctor.sh`

Checks that a development environment is set up correctly, and is the fastest way to find out why
something isn't working. It resolves every tool ESW needs — `cmake`, `ninja`, `git`, `uv`, the
`arm-none-eabi-*` cross-compilers, `STM32_Programmer_CLI`, `ST-LINK_gdbserver`, `STM32CubeMX`,
`STM32CubeProgrammer`, `clang-format`, `shellcheck` — and reports the version and location of
each, rather than stopping at the first thing it can't find.

```bash
./scripts/doctor.sh [--build] [--verbose]
```

Beyond tool presence it checks that:

- `arm-none-eabi-gcc` resolves inside STM32CubeCLT rather than the fallback toolchain, and that its
  version matches the one CI builds with. A fallback that has quietly taken over means CubeCLT's
  `PATH` entry has gone stale — usually after a CubeCLT upgrade.
- Every directory named in `/etc/profile.d/mrover-esw.sh` still exists, **and** is actually on the
  `PATH` of the shell you ran `doctor.sh` from. The second half catches the common case of a shell
  that predates the profile — see the note in
  [STM32Cube\*](../getting-started/stm32cube/index.md#downloading-and-installing-the-cube-tools-linux)
  for why zsh makes that easy to hit.
- No leftover hand-written `/etc/profile.d` snippet from the old manual setup is competing with it
  for `PATH` precedence. (ST's own `cubeclt-bin-path_*.sh`, installed by the CubeCLT package, is
  expected and ignored.)
- VS Code is installed and every extension in `.vscode/extensions.json` is present.
- The launcher entries exist and still point at binaries that are there — an upgrade that
  relocates an install shows up here. STM32CubeIDE is reported when installed and reported as
  skipped when not, without counting as a warning.
- The `lib/stm32g4/STM32CubeG4` submodule is initialized.
- `tools/.venv` is in sync with `tools/uv.lock`.

Missing tools are errors and exit non-zero; a fallback compiler, a stale `PATH` profile, a missing
programmer GUI and broken launcher entries are warnings, since none of them stops a build on its
own. The desktop-entry and `profile.d` checks are Linux-only and are skipped elsewhere.

- `--build`

Additionally runs an end-to-end build of `src/tests/logger` and confirms an `.elf` links. This is
the real proof that CubeCLT is installed correctly — it exercises the cross-compiler, the CMake
toolchain file, and the DBC/Python codegen in one shot. On failure the tail of the build log is
printed.

- `--verbose`

Prints the full build log instead of the last 20 lines.

### `venv.sh`

Syncs the `tools/.venv` Python virtual environment from `tools/uv.lock` by running
`uv sync --project tools`. This is purely a convenience: every script that needs Python runs
through `uv run`, which creates and syncs the environment on demand, so nothing breaks if you
forget it. Run it to pre-warm the environment, or standalone instead of the full `bootstrap.sh`
if the rest of the toolchain is already installed.

```bash
./scripts/venv.sh
```

### `new.sh`

The `scripts/new.sh` script is designed to aid the creation of new STM32 projects
that leverage the HAL libraries provided by STM. The script can be run as follows.

```bash
./scripts/new.sh --mcu <mcu> --src <path-to-new-project> [--lib <library>]
# OR
./scripts/new.sh --board <board> --src <path-to-new-project> [--lib <library>]
# OR
./scripts/new.sh --help  # to display the options menu
```

In the invocation above, the script accepts either an MCU (e.g. `STMG431CBTx`)
or a development board (e.g. `NUCLEO-G431RB`). 

!!! note
    Note that under the hood, this script emplaces this parameter directly into 
    a headless STM32CubeMX script, which will fail if the MCU or board provided
    is invalid. Use the STM32CubeMX project creation wizard to see possible MCU
    options available, and to verify the correct part information.

Under the hood, this script will run CubeMX and create an STM32 project for
the specified MCU. It does this by running a script of STM32CubeMX commands
that create the project in the correct CMake/GCC configuration, and then
manually patches the IOC to ensure some settings that aren't linked to
STM32CubeMX commands are correctly set.

The directory provided by the `--src` flag will be used for the creation of the
new project, and once the script finishes execution the project should be able
to be built.

Libraries can be provided with the `--lib` flag; if, for example, the new project
should link with ESW's library for commonly used stm32 header files (defined by
`lib/stm32/CMakeLists.txt`) and the CAN messages defined by the auto-generated
DBC headers (defined by `lib/dbc/CMakeLists.txt`), the script should be run with
`... --lib stm32 --lib dbc` options. This will result in the created
`<src>/CMakeLists.txt` linking against these libraries. This is not necessary to
get all the libraries correct at this point, as the file can be modified later.

The created `<src>/CMakeLists.txt` will then contain the following section.
```cmake
# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    stm32
    dbc
)
```

!!! note
    Future calls to the automated python tools that manage the auto-generated CMake
    files will overwrite this section, so these parameters will need to be provided
    to these scripts as well.

### `build.sh`

The build script is a wrapper around the CMake and the GCC distribution bundled with
the STM32CubeCLT, as well as the STMCubeProgrammer CLI. The build script can be run
in the following configurations.

1\. Clean Project

The script's `--clean` flag will remove all build artifacts from the project source.
This may be necessary to run after some updates to build files, as CMake is only
configured once and the cached build artifacts are used after.

```bash
./scripts/build.sh --src <src> --clean
```

2\. Build Project

The default behavior of the script is to build the specified project using CMake.
The script consumes the `--src <path-to-project>` flag to denote the directory
containing the project. If the project target is different from the directory name
(which is not the case for projects generated with the `./scripts/new.sh` script)
the `--target <target>` is provided. Additionally, the build preset can be altered
with the `--preset <preset>` flag. The default preset provided with the CMake
configuration is `Debug`, but `Release` is also valid. If no preset is specified,
the script will default to `Debug`. To build a project, the script can be run as
follows.

```bash
./scripts/build.sh --src <path-to-project> [--preset <preset>] [--target <target>]
```

3\. Flash Project

To flash the executable file for a project to an MCU, use the `--flash` flag. When
set, the script will attempt a build as above, and if successful invoke the
STM32CubeProgrammer CLI to connect to an ST-LINK and flash the executable via SWD.

!!! important
    Presently the script will only support flashing STM32 MCUs with SWD, and not JTAG.

```bash
./scripts/build.sh --src <path-to-project> [--preset <preset>] --flash
```

Finally, the build script will ensure the existance of a `<src>/.clangd` file for
development environment compatability. This needs to be generated per-project as
it contains some project-specific and system-specific parameters.

### `style.sh`

The style script enforces code style across all ESW code. It runs in CI, and must
be passing for PRs to be accepted. The script can be run with the following arguments.

```bash
./scripts/style.sh [--format] [--lint] [--fix] [--verbose]
```

If no options are provided, the `--format` run configuration will be used by default.
The functionality of the options is enumerated below.

- `--format`

This configuration will run `clang-format` and `ruff` to format `C/C++` and `Python`
code, respectively.

- `--lint`

This configuration will run linters `ruff` and `shellcheck` for `Python` and `shell`
scripts, respectively. This configuration will also run `ty` for static type analysis
of `Python` scripts.

- `--fix`

This configuration will fix all possible formatting and linting issues found with the
associated flags the script is run with. If the script finishes unsuccessfully with
this flag, there are issues with the codebase that the tools cannot automatically
resolve.

- `--verbose`

This flag runs the script with verbose output.

### `fdcanusb.sh`

This script is a wrapper over MJBot's `fdcanusb_daemon`. It is designed to run with
the MJBots FDCANUSB to allow connectivity to CAN networks over USB. The script is run
as follows.

```bash
sudo ./scripts/fdcanusb.sh --net <vcan-network>
```

!!! important
    This script must be run as `sudo` on linux systems to allow the virtual CAN
    network interface to be created and started.

The `--net` flag specifies the virtual CAN network the fdcanusb should use. To work
with the ROS2 stack, this should be something like `can[0-3]`. The script runs the
underlying `fdcanusb_daemon` in verbose mode, so the full CAN frame of all messages
on the bus will be directed to standard output.

!!! TODO
    MJBots no longer supports the `fdcanusb_daemon` (this script currently pulls the source
    for this from an archive). This script should be updated to not depend on this binary.

### `docs.sh`

Starts a local `zensical serve` server to preview these docs. See
[Building These Docs](../extra/build-docs.md).

```bash
./scripts/docs.sh
```

### `cmake_cfg.sh`

Wraps `tools/scripts/update_cmake_cfg.py`, which re-creates the auto-generated CMake files for a
project. Accepts `--src <path>` and any number of `--lib <name>` flags, passed straight through.

```bash
./scripts/cmake_cfg.sh --src <path-to-project> [--lib <library>]
```

### `monitor.sh`

Wraps `tools/scripts/monitor.py`, which displays serial log data sent from the MCU over the
ST-LINKv3's VCP-TX/VCP-RX pins. Accepts `--baud` and `--log-level`, passed straight through.

```bash
./scripts/monitor.sh [--baud <rate>] [--log-level <level>]
```

## Python Tools

The ESW python tools (located in `tools/`) are designed to enable rapid development, testing, 
and validation of the embedded hardware and software stack. Currently, the following submodules of 
the main `esw` python module are stable. Other submodules (e.g. `esw.bmc`) are product-specific,
and not documented here.

1\. `esw.stlink`

This module contains scripts to handle serial data from the VCP-TX and VCP-RX pins on the ST-LINKv3.
This enables serial data to be sent and received from the MCU via UART. The `tools/scripts/monitor.py`
script (wrapped by `scripts/monitor.sh`) will use this stack to display data sent from the MCU
with the logger, provided to the firmware via the `util` CMake package.

2\. `esw.cubemx`

This module contains scripts to handle the creation and management of STM32 CMake projects.
The `tools/scripts/generate_project.py` script is uesd by the `scripts/new.sh` script, as is
the `tools/scripts/update_cmake_cfg.py` script, which re-creates all CMake files for a project.

3\. `esw.can`

This module contains the functionality to use both the DBC files as well as the FDCANUSB to send
and receive can messages. Many scripts in `tools/scripts` rely on this functionality to send and
receive CAN messages to the MCUs. The `tools/scripts/can_header_gen.py` script is used by the
`lib/dbc/CMakeLists.txt` file to generate DBC header files for the `dbc` library.
