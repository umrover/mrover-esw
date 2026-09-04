# `esw.stlink`

Reads log output from a board over the ST-LINKv3's virtual COM port (VCP). This is the counterpart to
the firmware logger in `lib/util/Inc/logger.hpp`.

## Port Discovery

```python
get_stlinkv3_port()
```

Scans serial ports and returns the ST-LINKv3 device path, or `None` if none is attached. Matches
either on a description containing `ST-Link`/`STLINK`, or on ST's USB vendor ID `0x0483` with a
known ST-LINK product ID. Cached, so unplugging and replugging within one run will not be picked
up.

## Streaming

```python
stream_serial_data(port_name: Path, baud_rate: int, log_level=logging.DEBUG)
```

Opens the port at 8N1 with a one second timeout and loops reading lines until interrupted. Decodes
as UTF-8, replacing malformed bytes rather than raising, so line noise does not kill the session.
`Ctrl+C` exits cleanly; a serial error is re-raised.

Output is re-leveled based on the prefix the firmware sends, so device log levels show up as
Python log levels:

| Device prefix | Logged as |
| --- | --- |
| `DEBUG:` | debug |
| `INFO:` | info |
| `WARNING:` | warning |
| `ERROR:` | error |
| anything else | info |

## Usage

Through the wrapper:

```bash
./scripts/monitor.sh --baud 115200 --log-level INFO
```

Or directly:

```bash
uv run --project tools python tools/scripts/monitor.py --baud 115200
```

| Flag | Default | Meaning |
| --- | --- | --- |
| `--baud`, `-b` | `115200` | must match the firmware's UART configuration |
| `--log-level`, `-l` | `INFO` | one of `DEBUG`, `INFO`, `WARNING`, `ERROR` |

The port is discovered automatically and is not a flag. If nothing is found, check that the board
is connected and that you have permission to read the device. On Linux that usually means being
in the `dialout` group.
