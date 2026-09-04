# Installer Drop-In Directory

`./scripts/bootstrap.sh` installs the STM32Cube tools from archives you download into this directory. ST gates all of them behind a free MyST account (any email address works) and serves them from URLs that change every release, so they cannot be fetched automatically.

Everything in here except this file is gitignored - **the archives are large and ST's license does not permit redistributing them**.

## What To Download

Download the required three into this directory before continuing past the pause in `./scripts/bootstrap.sh`. STM32CubeIDE is optional.

| Tool | Download page | Pick | Name must contain |
| --- | --- | --- | --- |
| STM32CubeMX | [stm32cubemx](https://www.st.com/en/development-tools/stm32cubemx.html) | **Linux** installer | `stm32cubemx` |
| STM32CubeProgrammer | [stm32cubeprog](https://www.st.com/en/development-tools/stm32cubeprog.html) | **Linux** installer | `stm32cubeprog` or `stm32cubeprg` |
| STM32CubeCLT | [stm32cubeclt](https://www.st.com/en/development-tools/stm32cubeclt.html) | **Debian Linux** bundle | `stm32cubeclt` |
| STM32CubeIDE *(optional)* | [stm32cubeide](https://www.st.com/en/development-tools/stm32cubeide.html) | **Debian Linux** bundle | `stm32cubeide` |

Leave the archives zipped and do not rename them. The names should look as follows:

```
SetupSTM32CubeMX-6.18.1-Lin-x86_64.zip
SetupSTM32CubeProgrammer_linux_64.zip
stm32cubeclt_1.22-Lin-Deb-x86_64.sh.zip
stm32cubeide_2.2.0-Lin-Deb-x86_64.sh.zip
```

On each download page the button is labelled *Get latest*, and you will be asked to log in and accept the license before the file download starts.

## Why Separate Tools

- **STM32CubeCLT** - the command line toolchain. Provides `arm-none-eabi-gcc` (the compiler `scripts/build.sh` and CI actually use), `ST-LINK_gdbserver`, and `STM32_Programmer_CLI`.
- **STM32CubeMX** - the graphical peripheral configurator. `scripts/new.sh` drives it to generate projects, and you open `<project>.ioc` in it to change a configuration.
- **STM32CubeProgrammer** - the flashing and memory-inspection GUI. CubeCLT ships only the CLI programmer, so this is a separate install. It is deliberately kept off `PATH` - its `bin/` contains a second `STM32_Programmer_CLI` that would shadow CubeCLT's - and is launched from the applications menu instead.
- **STM32CubeIDE** *(optional)* - used only as a graphical debugger: breakpoints, watch expressions, call stack, live registers and the peripheral (SFR) view, against an ELF that `scripts/build.sh` produced. It is not used to build. See [Debugging with STM32CubeIDE](../docs/getting-started/stm32cube/index.md#debugging-with-stm32cubeide).

## Upgrading

Download the newer archive into this directory and re-run `./scripts/bootstrap.sh`. Only the tool whose archive changed is reinstalled.

See [the STM32Cube\* guide](../docs/getting-started/stm32cube/index.md) for the full setup walkthrough.
