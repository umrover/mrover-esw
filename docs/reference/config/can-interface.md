# CAN Configuration Interface

`tools/scripts/config.py` pushes a device's settings to a board over CAN. This is how a board is
commissioned after it has been flashed.

## Prerequisites

A CAN interface must be up. With an mjbots FDCANUSB:

```bash
sudo ./scripts/fdcanusb.sh --net can0
```

You also need the board's **current** CAN ID. A freshly flashed board has whatever its firmware
defaults to, not the ID in the file you are about to send.

## Usage

```bash
uv run --project tools python tools/scripts/config.py \
    --definition config/abs.yaml \
    --file rover/ra/abs_de_pitch.yaml \
    --can can0 \
    --id 55
```

!!! warning
    On new MCUs, the flash defaults high, so the address of a new device will be `0xFF`, or `255`.

| Flag | Required | Meaning |
| --- | --- | --- |
| `--definition`, `-d` | yes | the board's register layout, from `config/` |
| `--file`, `-f` | yes | the device's values, from `rover/` |
| `--can`, `-c` | yes | CAN interface name, e.g. `can0` |
| `--id`, `-i` | yes | the board's current CAN ID |
| `--read`, `-r` | no | read config back from the device |

!!! warning
    `--read` is **not implemented**. The flag parses and the tool runs, but the read branch is an
    empty stub, so it reports success without having read anything.

## Sent Configuration Frames

1. Parses the definition to work out each register's address, type and bit layout.
2. Reads the device file and packs its values into register-sized integers. Floats are
   reinterpreted as their IEEE-754 bit pattern.
3. Prints the resulting register table so you can check it before it goes out.
4. Sends one `ESWConfigCmd` frame per register, addressed to `--id`, with a 100 ms gap between
   frames.

Each frame carries three signals:

| Signal | Width | Meaning |
| --- | --- | --- |
| `address` | 8 bits | register address |
| `value` | 32 bits | raw value |
| `apply` | 1 bit | commit the write |

## Processing Configuration Frames

Firmware handles `ESWConfigCmd` by calling `set_raw(address, value)` on its config struct. That
finds the register at that address, writes it, and persists the change: the last flash page is
read into RAM, patched, erased and reprogrammed.

Because each register is a separate frame and each write rewrites the flash page, configuration is
not atomic. A board interrupted partway through has some new values and some old ones. Re-run the
tool rather than assuming it took.

## Changing a Board's CAN ID

`can_id` is itself a register, so changing it is an ordinary write, but the board stops answering
on the old ID the moment it lands. Use the new ID for anything afterwards, and if the run fails
partway through, retry with whichever ID actually took.
