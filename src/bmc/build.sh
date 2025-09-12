#!/usr/bin/env zsh

PROJECT="bmc"
CMAKE_PRESET="Debug"

# configure build
if [ ! -d "build/$CMAKE_PRESET" ]; then
    cmake --preset "$CMAKE_PRESET"
fi

# execute build
cmake --build --target "$PROJECT" --preset "$CMAKE_PRESET"

