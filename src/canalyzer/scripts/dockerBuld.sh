#!/bin/bash
set -euo pipefail

IMAGE_NAME="can_logger_image"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_CONTEXT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
DOCKER_FILE="$BUILD_CONTEXT/src/canalyzer/Core/Dockerfile"

if [ ! -f "$DOCKER_FILE" ]; then
    echo "Error: Dockerfile not found at '$DOCKER_FILE'"
    exit 1
fi

echo "Building CAN code image..."
docker build -t "$IMAGE_NAME" -f "$DOCKER_FILE" "$BUILD_CONTEXT"
echo "CAN code image build complete"