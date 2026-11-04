#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME [ansible-playbook options]

Install and configure ESW development environment via ansible.

options:
  -h, --help            show this help message
EOF
    exit "${1:-0}"
}

case "${1:-}" in
    -h|--help) usage ;;
esac

case "$(uname -s)" in
    Linux)
        command -v apt-get > /dev/null 2>&1 \
            || die "on Linux this script supports Debian/Ubuntu only (\`apt-get\` not found)"
        if ! command -v ansible-playbook > /dev/null 2>&1; then
            info "installing ansible..."
            sudo apt-get update
            sudo apt-get install -y ansible
        fi
        ;;
    Darwin)
        command -v brew > /dev/null 2>&1 \
            || die "\`brew\` is required on macOS, install from https://brew.sh and re-run"
        if ! command -v ansible-playbook > /dev/null 2>&1; then
            info "installing ansible..."
            brew install ansible
        fi
        ;;
    *)
        die "unsupported operating system: $(uname -s) (Linux and macOS only)"
        ;;
esac

require ansible-playbook

export ANSIBLE_CONFIG="$ESW_ROOT/ansible/ansible.cfg"

ansible-playbook "$ESW_ROOT/ansible/bootstrap.yml" --connection=local --ask-become-pass "$@"
