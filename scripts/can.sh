#!/usr/bin/env zsh

set -euo pipefail

source "${0:A:h}/common.sh"

# based on bus timing of BMC
readonly BITRATE=1000000
readonly SAMPLE_POINT=0.671
readonly SJW=10
readonly DBITRATE=5000000
readonly DSAMPLE_POINT=0.706
readonly DSJW=5

# stlink interface for flashing (with stlink-v2 USB)
STLINK_PORT="${STLINK_PORT:-swd}"
STLINK_FREQ="${STLINK_FREQ:-8000}"
STLINK_RESET="${STLINK_RESET:-HWrst}"

# parameter inputs
MODE=""
NET=""
SERIAL=""
TERMINATION=""
DUMP=false
WATCH=false
FIRMWARE=""

usage() {
    cat <<EOF
Usage: $SCRIPT_NAME --list
       sudo $SCRIPT_NAME --net <canN> [--serial <usb-serial>] [--term on|off] [--dump] [--watch]
       $SCRIPT_NAME --flash-fw <firmware.elf>

Open gs_usb FDCAN adapter as a SocketCAN interface.

options:
  -l, --list                list adapters, their interfaces and serials
  -n, --net <canN>          name the adapter <canN> and bring the interface up
  -s, --serial <serial>     with \`--net\`: the adapter to use
  -t, --term <on|off>       with \`--net\`: switch the adapter's 120 ohm termination
  -d, --dump                with \`--net\`: show the most recent frames, error frames included
  -w, --watch               with \`--net\`: show the bus state, error counters and statistics
  --flash-fw <file.elf>     flash gs_usb firmware onto an adapter over SWD (ST-LINK)
  -h, --help                show this help message
EOF
    exit "${1:-0}"
}

set_mode() {
    [[ -z "$MODE" || "$MODE" == "$1" ]] || usage_error "choose one of \`--list\`, \`--net\` or \`--flash-fw\`"
    MODE="$1"
}

# parse flags
while [[ $# -gt 0 ]]; do
    case "$1" in
        -l|--list)      set_mode list; shift ;;
        -n|--net)       need_value "$1" $#; set_mode net; NET="$2"; shift 2 ;;
        -s|--serial)    need_value "$1" $#; SERIAL="$2"; shift 2 ;;
        -t|--term)      need_value "$1" $#; TERMINATION="$2"; shift 2 ;;
        -d|--dump)      DUMP=true; shift ;;
        -w|--watch)     WATCH=true; shift ;;
        --flash-fw)     need_value "$1" $#; set_mode flash; FIRMWARE="${2:A}"; shift 2 ;;
        -h|--help)      usage ;;
        *)              unknown_option "$1" ;;
    esac
done

# check input combinations
[[ -n "$MODE" ]] || usage_error "one of \`--list\`, \`--net\` or \`--flash-fw\` is required"
if [[ "$MODE" != "net" ]]; then
    [[ -z "$SERIAL" && -z "$TERMINATION" && "$DUMP" == "false" && "$WATCH" == "false" ]] \
        || usage_error "\`--serial\`, \`--term\`, \`--dump\` and \`--watch\` only apply to \`--net\`"
else
    [[ "$NET" =~ '^[A-Za-z0-9_-]{1,15}$' ]] || usage_error "\`$NET\` is not a valid interface name"
    [[ -z "$TERMINATION" || "$TERMINATION" == (on|off) ]] || usage_error "\`--term\` takes \`on\` or \`off\`, not \`$TERMINATION\`"
    ((EUID == 0)) || usage_error "\`--net\` configures the interface, run with \`sudo\`"
fi
[[ "$MODE" == "flash" || "$(uname -s)" == "Linux" ]] || die "SocketCAN needs Linux"

# netdev_attr <interface> <usb device attribute>
netdev_attr() {
    cat "/sys/class/net/$1/device/../$2" 2> /dev/null || print -r -- "?"
}

# populate adapter sets
# ADAPTERS - new gs_usb interfaces
# OLD_ADAPTERS - mjbots adapters still on old firmware that requires daemon
find_adapters() {
    local dev iface
    ADAPTERS=()
    for dev in /sys/class/net/*(N); do
        [[ "$(readlink -f "$dev/device/driver" 2> /dev/null)" != */gs_usb ]] || ADAPTERS+=("${dev:t}")
    done
    OLD_ADAPTERS=()
    for dev in /sys/bus/usb/devices/*(N); do
        [[ "$(cat "$dev/manufacturer" 2> /dev/null)" == "mjbots" ]] || continue
        [[ "$(cat "$dev/product" 2> /dev/null)" == "fdcanusb" ]] || continue
        for iface in "$dev/${dev:t}":*(N); do
            [[ ! -d "$iface/net" ]] || continue 2
        done
        OLD_ADAPTERS+=("$(cat "$dev/serial" 2> /dev/null) $(<"$dev/idVendor"):$(<"$dev/idProduct")")
    done
}

# display list of all adapters
list_adapters() {
    local dev old
    section "gs_usb adapters"
    for dev in "${ADAPTERS[@]}"; do
        pass "${(r:8:)dev} serial ${(r:12:)$(netdev_attr "$dev" serial)} ${(r:7:)$(</sys/class/net/$dev/operstate)} $(netdev_attr "$dev" product)"
    done
    ((${#ADAPTERS})) || note "none"
    for old in "${OLD_ADAPTERS[@]}"; do
        warn "serial ${old% *} (${old#* }) runs old firmware: reflash with \`--flash-fw\`"
    done
}

# live view until ctrl-c
# bus state on top, the most recent frames below
live_view() {
    local rows cols lines big
    DUMP_LOG="$(mktemp)"
    DUMP_PID=""
    trap '[[ -z "$DUMP_PID" ]] || kill "$DUMP_PID" 2> /dev/null; rm -f "$DUMP_LOG"; screen_close' EXIT
    trap 'exit 0' INT TERM
    if [[ "$DUMP" == "true" ]]; then
        require candump stdbuf
        # candump all frames with delta timestamps
        stdbuf -oL candump -td -e "$NET,0:0,#FFFFFFFF" >> "$DUMP_LOG" 2>&1 &
        DUMP_PID=$!
    fi

    # 20Hz refresh
    zmodload zsh/zselect
    resize() { rows=$(tput lines) cols=$(tput cols); }
    resize
    trap resize WINCH
    screen_open
    while true; do
        lines=()
        [[ "$WATCH" == "false" ]] || lines+=("${(@f)$(ip -details -statistics link show "$NET" | expand)}")
        if [[ "$DUMP" == "true" ]]; then
            lines+=("${(r:cols::─:):-── frames on $NET, most recent last (ctrl-c to stop) }")
            ((rows - ${#lines} - 1 < 1)) || lines+=("${(@f)$(tail -n $((rows - ${#lines} - 1)) "$DUMP_LOG" | expand)}")
            big=(${~DUMP_LOG}(NLk+16384))
            ((! ${#big})) || : > "$DUMP_LOG"
        fi
        screen_draw "$cols" "${lines[@]:0:$((rows - 1))}"
        zselect -t 5 || true
    done
}

case "$MODE" in
    list)
        find_adapters
        list_adapters
        ;;

    net)
        require ip

        # select adapter
        find_adapters
        DEV=""
        if [[ -n "$SERIAL" ]]; then
            for dev in "${ADAPTERS[@]}"; do
                [[ "$(netdev_attr "$dev" serial)" != "$SERIAL" ]] || DEV="$dev"
            done
            [[ -n "$DEV" ]] || { list_adapters >&2; die "no gs_usb adapter with serial \`$SERIAL\`"; }
        elif ((${#ADAPTERS} == 1)); then
            DEV="${ADAPTERS[1]}"
        elif ((${#ADAPTERS} == 0)); then
            ((! ${#OLD_ADAPTERS})) || { list_adapters >&2; die "adapter needs gs_usb firmware: \`./scripts/$SCRIPT_NAME --flash-fw <firmware.elf>\`"; }
            die "no gs_usb CAN adapter found"
        else
            list_adapters >&2
            die "several adapters found, choose one with \`--serial\`"
        fi

        if [[ "$DEV" != "$NET" ]]; then
            [[ ! -e "/sys/class/net/$NET" ]] || die "\`$NET\` already belongs to another device"
            info "renaming $DEV to $NET"
            ip link set "$DEV" down
            ip link set "$DEV" name "$NET"
        fi

        CAN_ARGS=(bitrate "$BITRATE" sample-point "$SAMPLE_POINT" sjw "$SJW")
        CAN_ARGS+=(dbitrate "$DBITRATE" dsample-point "$DSAMPLE_POINT" dsjw "$DSJW" fd on)
        case "$TERMINATION" in
            on)     CAN_ARGS+=(termination 120) ;;
            off)    CAN_ARGS+=(termination 0) ;;
        esac
        ip link set "$NET" down
        ip link set "$NET" type can "${CAN_ARGS[@]}"
        ip link set "$NET" up

        # summarize interface as configured
        DETAILS="$(ip -details link show "$NET")"
        STATE="?"
        TERM_OHMS="?"
        [[ ! "$DETAILS" =~ 'state ((ERROR|BUS)-[A-Z]+)' ]] || STATE="${match[1]}"
        [[ ! "$DETAILS" =~ 'termination ([0-9]+)' ]] || TERM_OHMS="${match[1]} ohm"
        section "$NET"
        pass "serial $(netdev_attr "$NET" serial), $STATE"
        note "bitrate $BITRATE (sample point $SAMPLE_POINT), dbitrate $DBITRATE (sample point $DSAMPLE_POINT)"
        note "termination $TERM_OHMS"
        ok "$NET is up"

        [[ "$DUMP" == "false" && "$WATCH" == "false" ]] || live_view
        ;;

    flash)
        [[ -f "$FIRMWARE" ]] || die "firmware \`$FIRMWARE\` not found"
        require STM32_Programmer_CLI

        info "flashing $FIRMWARE over $STLINK_PORT"
        STM32_Programmer_CLI --connect port="$STLINK_PORT" freq="$STLINK_FREQ" reset="$STLINK_RESET" \
            --write "$FIRMWARE" --verify --start
        ok "flashed, replug the adapter and check \`./scripts/$SCRIPT_NAME --list\`"
        ;;
esac
