#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

CACHE_DIR="$TOOLS_DIR/fdcanusb"
SOURCE_URL="https://raw.githubusercontent.com/mjbots/fdcanusb/4361dc321262693fa2548d8d4245fe1161b65f39/sw/fdcanusb_daemon.cc"
UDEV_SOURCE_URL="https://raw.githubusercontent.com/mjbots/fdcanusb/refs/heads/master/70-fdcanusb.rules"
SOURCE_FILE="${CACHE_DIR}/fdcanusb_daemon.cc"
BINARY="${CACHE_DIR}/fdcanusb_daemon"
IFACE=""

usage() {
    cat <<EOF
Usage: sudo $SCRIPT_NAME --net <vcan-network>

Run FDCANUSB daemon and vcan network. Installs and manages FDCANUSB tooling.

options:
  -n, --net <vcan-network>      set vcan interface
  -h, --help                    show this help message
EOF
    exit "${1:-0}"
}

# parse parameters
while [[ $# -gt 0 ]]; do
    case "$1" in
        -n|--net)       need_value "$1" $#; IFACE="$2"; shift 2 ;;
        -h|--help)      usage ;;
        *)              unknown_option "$1" ;;
    esac
done

# verify network interface provided
[[ -n "$IFACE" ]] || usage_error "\`--net\` required"

# enforce sudo
((EUID == 0)) || usage_error "run this script as root"

require curl ip udevadm

# create cache directory
if [[ ! -d "$CACHE_DIR" ]]; then
    mkdir -p "$CACHE_DIR"
    info "created cache directory at ${CACHE_DIR}"
fi

# download fdcanusb daemon
if [[ ! -f "$SOURCE_FILE" ]]; then
    curl -L "$SOURCE_URL" -o "$SOURCE_FILE" || die "could not download source from $SOURCE_URL"
    info "downloaded fdcanusb daemon source to $SOURCE_FILE"
fi

# build fdcanusb daemon
if [[ ! -f "$BINARY" ]]; then
    require g++
    g++ -Wall -g -O2 "$SOURCE_FILE" -o "$BINARY"
    info "built fdcanusb daemon"
fi

# install and load udev rules
if [[ ! -f "/etc/udev/rules.d/70-fdcanusb.rules" ]] && [[ ! -f "/lib/udev/rules.d/70-fdcanusb.rules" ]]; then
    curl -L "$UDEV_SOURCE_URL" -o "/etc/udev/rules.d/70-fdcanusb.rules" || die "could not download source from $UDEV_SOURCE_URL"
    info "installing udev rules"
    udevadm control --reload-rules
    udevadm trigger --subsystem-match=tty
    sleep 1
    info "installed udev rules for fdcanusb"
fi

# verify fdcanusb exists
[[ -e "/dev/fdcanusb" ]] || die "fdcanusb device not found at /dev/fdcanusb"

# setup virtual CAN network interface
if ! ip link show "$IFACE" > /dev/null 2>&1; then
    info "creating vcan interface $IFACE"
    ip link add name "$IFACE" type vcan || true
    info "created $IFACE"
fi

# start vcan network
ip link set "$IFACE" up

# run fdcanusb daemon in foreground
"${BINARY}" -F -v "/dev/fdcanusb" "$IFACE"
