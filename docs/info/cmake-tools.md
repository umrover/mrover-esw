# CMake + CubeMX/CubeCLT Toolchain

1. Install [CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html#get-software). This is the `.ioc` file editor.
2. Install [CubeCLT](https://www.st.com/en/development-tools/stm32cubeclt.html#get-software). This is the command-line toolset that contains all the software necessary to build and deploy the codebase.
3. In order to use the scripts to build and flash, add all `*/bin` directories from CubeCLT to `$PATH`.
4. Install [CubeMCUFinder](https://www.st.com/en/development-tools/st-mcu-finder-pc.html#get-software).
5. Install the corresponding extension for your editor. In each case, the extension will need to be provided the location to CubeMX and CubeCLT.
	1. [CLion](https://www.jetbrains.com/help/clion/embedded-stm32.html)
	2. [VS Code](https://www.st.com/content/st_com/en/campaigns/stm32-vs-code-extension-z11.html)
6. `./scripts/build.sh --src <PROJECT>` will build the project
7. `./scripts/flash.sh --src <PROJECT>` will flash the project to the MCU connected to the STLINK

## Adding CMake Dependencies to Projects

To add `lib/` dependencies to a CubeMX project, add the following to `CMakeLists.txt`.

```cmake
# Add fwlib dependency
add_subdirectory(../../lib fwlib)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    stm32
    units
    util
)
```
