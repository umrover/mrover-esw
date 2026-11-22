# Continuous Integration

Four workflows live in `.github/workflows/`. Firmware CI runs the same `scripts/build.sh` that
developers run, inside a container built from `Dockerfile.arm-gnu`.

## What Gets Built

`ci.json` at the repo root is the build matrix:

```json
[
    "src/tests/serial",
    "src/tests/logger",
    "src/bmc",
    "src/pdlb",
    "src/abs"
]
```

Each entry is built at both `Debug` and `Release`, so ten builds per run, with `fail-fast: false`.

## `ci.yml`

Three jobs, the latter two running inside the arm-gnu container:

| Job | What it does |
| --- | --- |
| `generate-test-matrix` | reads `ci.json` into a job output |
| `style` | `uv sync --project tools --locked`, then `./scripts/style.sh --format --lint` |
| `test-build` | the `dir` x `preset` matrix, calling `./scripts/build.sh` |

The style job runs in check-only mode, so formatting problems fail rather than being fixed. Run
`./scripts/style.sh --format --lint --fix` locally before pushing.

Checkout uses `submodules: recursive`, which `build.sh` requires.

## `docker.yml`

A reusable workflow that builds and publishes `ghcr.io/<repo>/arm-gnu`. It rebuilds only when
`Dockerfile.arm-gnu` or the workflow itself changed, and is skipped entirely for fork PRs.

`Dockerfile.arm-gnu` is the **single version pin for two tools**:

```dockerfile
FROM ghcr.io/astral-sh/uv:0.12.8 AS uv
```

```dockerfile
ENV ARM_GNU_LINK=https://.../arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
```

The `FROM ... AS uv` stage exists specifically so Dependabot, which parses only `FROM` lines, can
bump the `uv` pin. `site.yml` greps that same line to keep its `setup-uv` in sync, and
`ansible/tasks/uv.yml` greps it to converge a developer's `uv` to the same version.

The image carries `cmake`, `ninja`, `uv` and the ARM toolchain, plus `clang-format` and
`shellcheck` for the style job. It deliberately has no CubeCLT and no `STM32_Programmer_CLI` --
CI never flashes.

## `release.yml`

Manually triggered with a semver tag. This is the one place CMake is invoked without a preset,
because it builds the `dbc` target for the host rather than the MCU:

```bash
cmake -S ./lib/dbc -B ./lib/dbc/build \
  -DVENV_PYTHON=$(pwd)/tools/.venv/bin/python \
  -DTOOLS_DIR=$(pwd)/tools
cmake --build ./lib/dbc/build --target dbc
```

`tools/CMakeLists.txt` is never added on this path, so `VENV_PYTHON` and `TOOLS_DIR` are injected
by hand and `lib/dbc/CMakeLists.txt` fabricates a no-op `python_env_ready` target to stand in.

The generated headers are packaged with `lib/dbc/release.CMakeLists.txt` into `mrover_can.tar.gz`,
so consumers get an `INTERFACE` target over pre-generated headers with no Python, no `cantools`
and no submodule.

## `site.yml`

Builds and deploys these docs. See [Maintaining Documentation](../maintaining-docs.md).
