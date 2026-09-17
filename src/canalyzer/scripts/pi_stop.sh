#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$(realpath "$SCRIPT_DIR/..")"
echo ">> [Cleanup] Shutting down services..."
docker compose down