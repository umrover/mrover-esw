# `esw.can`

CAN bus access and DBC handling. Every tool that talks to a board goes through this module.

## `esw.can.canbus`

### CANBus

A context manager wrapping `python-can` and a `cantools` database. Opens a SocketCAN interface in
FD mode with bit rate switching enabled.

```python
CANBus(dbc: Database, channel: str, on_recv: Callable = lambda arg: None)
```

| Method | Signature |
| --- | --- |
| `send` | `send(message_name: str, signals: dict[str, Any], src_id: int = 0, dest_id: int = 0) -> None` |
| `recv` | `recv(block: bool = True, timeout: float \| None = None) -> Tuple[str, dict, int, int] \| None` |
| `get_dbc` | `get_dbc() -> Database` |

`recv` returns `(message_name, signals, src_id, dest_id)`.

```python
from esw.can.canbus import CANBus
from esw.can.dbc import get_dbc

with CANBus(get_dbc(dbc_name="MRoverCAN"), "can0") as bus:
    bus.send("BMCResetCmd", {"reset": 1, "clear_faults": 1}, dest_id=49)
    print(bus.recv(timeout=1.0))
```

### Arbitration ID Layout

Messages carry a source and destination node ID packed into the arbitration ID alongside the DBC
frame ID:

```
arbitration_id = frame_id + (src_id << 8) + dest_id
```

| Constant | Value |
| --- | --- |
| `_CAN_DEST_ID_MASK` | `0x00FF`, offset 0 |
| `_CAN_SRC_ID_MASK` | `0xFF00`, offset 8 |
| `_MJBOTS_CAN_PREFIX` | `0x0000` |

The same constants are emitted into the generated C++ header, so both sides agree. Frames with a
base ID of `0x0000` are moteus traffic and are logged but not decoded.

### `float2bits`

```python
float2bits(value: float)
```

Reinterprets a float as its IEEE-754 bit pattern. Needed when sending a float through an integer
DBC signal, which is what the configuration interface does. Raises `TypeError` on non-floats.

## `esw.can.dbc`

### `get_dbc`

```python
get_dbc(filepath: Path | None = None, dbc_name: str | None = None) -> Database
```

Loads a DBC into a `cantools` database. With `dbc_name`, resolves `<repo root>/dbc/<name>.dbc`;
with `filepath`, loads that path directly. One of the two is required. Cached, so repeated calls
with the same argument are free.

### Header Generation

```python
generate_can_header(ctx: Path, dest: Path, files: list[str]) -> None
```

Renders `<ctx>/templates/dbc_header.hpp.j2` once per DBC file into `<dest>/<stem>.hpp`. Invoked
from `lib/dbc/CMakeLists.txt` during a firmware build, not usually by hand. See
[Generated Libraries](../build/codegen.md).

!!! warning
    `generate_can_header` catches every exception and prints it rather than raising. A malformed
    DBC produces a passing build with a missing or stale header, and the real failure surfaces
    later as a C++ compile error.

Two helpers back the template context:

- `get_c_type(signal) -> str` maps a DBC signal to a C type, choosing width by bit length and
  `float`/`double` at the 32-bit boundary.
- `prepare_context(dbc_db, dbc_name)` builds the full Jinja context: messages keyed by frame ID,
  per-signal types and lengths, and the sorted message name list.

## DBC File

`dbc/MRoverCAN.dbc` is the single source of truth for the CAN protocol. It is consumed twice: by this module at runtime, and by the firmware build to generate
C++ message classes.

Per-board message tables are on the [board pages](../../projects/boards/index.md).

A tagged release packages the generated headers as `mrover_can.tar.gz` for the ros2 build, which needs the protocol without the Python toolchain.
See [Continuous Integration](../build/ci.md#releaseyml).
