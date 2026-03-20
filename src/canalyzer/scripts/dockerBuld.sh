#!/bin/bash
set -e
IMAGE_NAME="can_logger_image"
PROJECT_REL_PATH="src/canalyzer"
BUILD_CONTEXT="../../.."
DOCKER_FILE="../Core/Dockerfile"
REPO_ROOT="mrover-esw"
DBC_PATH="dbc/"

if [ ! -d "$BUILD_CONTEXT" ]; then
    echo "Error: build directory '$BUILD_CONTEXT' not found"
    exit 1
fi

RESOLVED_DIR=$(basename "$(realpath "$BUILD_CONTEXT")")
if [ "$RESOLVED_DIR" != "$REPO_ROOT" ]; then
    echo "Error: build context must be the '$REPO_ROOT' directory, but got '$RESOLVED_DIR'"
    exit 1
fi

echo "Building CAN code image..."
docker build -t "$IMAGE_NAME" -f "$DOCKER_FILE" \
    --build-arg PROJECT_REL_PATH="$PROJECT_REL_PATH" \
    --build-arg DBC_PATH="$DBC_PATH" \
    "$BUILD_CONTEXT" 
echo "CAN code image build complete"