#!/usr/bin/env bash

set -euo pipefail

RED="\e[1;31m"
BLUE="\e[1;34m"
NC="\e[0m"

ESW_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

die() {
    printf "%b\n" "${RED}✗ error: $1${NC}" >&2
    exit 1
}

case "$(uname -s)" in
    Linux)
        command -v apt-get > /dev/null 2>&1 \
            || die "on Linux this script supports Debian/Ubuntu only (apt-get not found)"
        if ! command -v ansible-playbook > /dev/null 2>&1; then
            printf "%b\n" "${BLUE}installing ansible...${NC}"
            sudo apt-get update
            sudo apt-get install -y ansible
        fi
        ;;
    Darwin)
        command -v brew > /dev/null 2>&1 \
            || die "Homebrew is required on macOS. Install it from https://brew.sh and re-run this script"
        if ! command -v ansible-playbook > /dev/null 2>&1; then
            printf "%b\n" "${BLUE}installing ansible...${NC}"
            brew install ansible
        fi
        ;;
    *)
        die "unsupported operating system: $(uname -s) (Linux and macOS only)"
        ;;
esac

export ANSIBLE_CONFIG="$ESW_ROOT/ansible/ansible.cfg"

ansible-playbook "$ESW_ROOT/ansible/bootstrap.yml" --connection=local --ask-become-pass
