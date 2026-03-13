#!/bin/bash

set -e

IMAGE_NAME="can_logger_image"
BUILD_CONTEXT="./can_code"

if [! -d "$BUILD_CONTEXT"]; then
    echo "Error: build directory "$BUILD_CONTEXT" not found"
    exit1
fi

echo "Building CAN code image..."
docker build -t "$imageName" "$buildContext"

echo "CAN code image build complete"