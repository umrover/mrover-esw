# Python Tools Overview

`tools/` holds the `esw` Python package and the scripts built on it. They cover code generation
for the firmware build, CAN communication, project scaffolding and serial monitoring.

## Layout

```
tools/
  pyproject.toml       package metadata, dependencies, ruff config
  uv.lock              locked dependency set
  .python-version      3.12
  esw/                 the package
    __init__.py        esw_logger, get_esw_root()
    can/               CAN bus access and DBC handling
    config/            board register definitions and value packing
    cubemx/            CubeMX project generation and CMake rendering
    stlink/            ST-LINKv3 serial monitoring
    visualization/     live plotting
  scripts/             command line entry points
```

## Running

The package is never installed globally and there is no activation step to remember. Everything
goes through `uv`, which creates and syncs `tools/.venv` on demand:

```bash
uv run --project tools python tools/scripts/monitor.py --baud 115200
```

Some scripts have a shell wrapper in `scripts/` that does exactly this:

| Wrapper | Script |
| --- | --- |
| `scripts/monitor.sh` | `monitor.py` |
| `scripts/new.sh` | `generate_project.py`, then `update_cmake_cfg.py` |
| `scripts/build.sh` | `clangd.py` |

Two more are invoked by CMake during a firmware build: `can_header_gen.py` and `config_gen.py`.
The rest are run by hand. See the [Script Reference](scripts.md).

## Virtual Environment

Four things can create or sync `tools/.venv`, all equivalent:

| Entry point | When it runs |
| --- | --- |
| `./scripts/venv.sh` | manually, or from the Ansible bootstrap |
| `.envrc` | on `cd` into the repo, if you use direnv |
| the `python_env_ready` CMake target | first firmware build |
| `uv run --project tools` | every time you run a script |

## Version Pinning

`tools/pyproject.toml` sets `required-version = ">=0.12"` for `uv` itself, which is a floor, not a
pin. The exact version everyone converges on lives in `Dockerfile.arm-gnu`:

```dockerfile
FROM ghcr.io/astral-sh/uv:0.12.8 AS uv
```

This matters because `uv` writes `tools/uv.lock` and the lockfile revision moves with the tool. An
older `uv` fails loudly rather than silently rewriting the lock at a revision CI cannot read.

## Style

`scripts/style.sh` runs `ruff format`, `ruff check` and `ty check` over `tools/`. CI runs it in
check-only mode, so fix problems before pushing:

```bash
./scripts/style.sh --format --lint --fix
```

Ruff is configured for a 120 character line length with the `E4`, `E7`, `E9`, `F`, `I` and `B`
rule sets. `ty` runs on defaults.
