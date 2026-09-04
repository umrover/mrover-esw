# mrover-esw

This repository contains all relevant information for the Embedded Software (ESW) MRover subteam.

## Repo Structure

Documentation can be found in `docs/` and can be viewed [here](https://umrover.github.io/mrover-esw/).
Instructions for developing and building the documentation locally can be found in the
[Maintaining Documentation](docs/reference/maintaining-docs.md) page.

The `src/` directory contains the source code for the ESW subteam. As of now, only firmware should be placed here.
All ROS2 code and device clients should go in the [mrover-ros2 repository](https://github.com/umrover/mrover-ros2).

The `lib/` directory contains library code including hardware drivers and utility code.

The `starter-projects/` directory contains the starter code for the new member starter projects.
Information about how to start the starter projects can be found in the documentation.

The `scripts/` directory contains various build and utility scripts.

The `tools/` directory contains our Python utilities. This includes scripts for generating new projects
and helpful CAN utlities. Python dependencies are managed with [uv](https://docs.astral.sh/uv/). There is
no setup step to remember: the scripts in `scripts/` run through `uv run`, which creates and syncs
`tools/.venv` on demand.

On a fresh Ubuntu/Debian machine, run `./scripts/bootstrap.sh` to install all of the above plus the ARM
toolchain and STM32CubeMX/CubeProgrammer/CubeCLT in one step; see
`docs/getting-started/stm32cube/index.md` for details. Run `./scripts/doctor.sh` afterwards to verify
the install.

The `dbc/` directory contains our CAN database files.

The `ci.json` file contains the paths for the continuous integration (CI) system to build and test the code.
We use GitHub Actions for our CI system and the configuration can be found in `.github/workflows/`.
