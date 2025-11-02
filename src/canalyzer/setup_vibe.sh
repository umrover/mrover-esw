#!/bin/bash
set -euo pipefail

# Globals
DOCKER_LOG="docker_compose.log"
DOCKER_STATUS_LOG="docker_status.log"
RUNNING=true

# Cleanup function
cleanup() {
    echo "Gracefully exiting at $(date)" | tee -a "$DOCKER_LOG"
    RUNNING=false
    docker-compose down
    exit 0
}

trap cleanup SIGINT SIGTERM

# Ensure directories exist with proper ownership
directoryExists() {
    for dir in "./grafana_data:472" "./influxdb_data:1500"; do
        path="${dir%%:*}"
        uid="${dir##*:}"

        if [ -d "$path" ]; then
            current_uid=$(stat -c "%U" "$path")
            if [ "$current_uid" != "$uid" ]; then
                echo "Adjusting permissions for $path..."
                chown "$uid:$uid" "$path"
            else
                echo "Ownership of $path already correct ($uid)"
            fi
        else
            echo "$path does not exist, creating..."
            mkdir -p "$path"
            chown "$uid:$uid" "$path"
        fi
    done
}

# Periodically log docker status
check_docker() {
    while $RUNNING; do
        echo "---- $(date) ----" >> "$DOCKER_STATUS_LOG"
        docker ps -a >> "$DOCKER_STATUS_LOG" 2>&1
        sleep 60
    done
}

main() {
    directoryExists
    # Start docker-compose in background and log output
    docker-compose up --build 2>&1 | tee -a "$DOCKER_LOG" &
    docker_pid=$!

    # Start docker status logging in background
    check_docker &
    docker_status_pid=$!

    # Wait for docker-compose to finish
    wait $docker_pid

    # Stop the background logger
    RUNNING=false
    wait $docker_status_pid
}

main
