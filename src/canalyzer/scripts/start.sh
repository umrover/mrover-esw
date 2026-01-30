#!/bin/bash
set -e

# -------------------------------------------------------
# Setup Directory
# -------------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$(realpath "$SCRIPT_DIR/..")"

# -------------------------------------------------------
# Cleanup Function
# -------------------------------------------------------
cleanup() {
    echo ""
    echo ">> [Cleanup] Shutting down services..."
    docker compose down
}

# Trap Ctrl+C (SIGINT) and EXIT
trap cleanup SIGINT EXIT

# -------------------------------------------------------
# Execution
# -------------------------------------------------------

echo ">> [Start] Starting Docker Compose (Silent Mode)..."

# 1. Start in detached mode (-d)
#    This runs the containers in the background and returns immediately.
docker compose up -d --build --remove-orphans

echo ">> System is running. Press Ctrl+C to stop."

# 2. The "Infinite Wait"
#    We need a command that does nothing forever so the script doesn't exit.
#    'sleep infinity' works on most Linux systems.
sleep infinity