# Absolute Encoder (ABS)

Reads a magnetic absolute encoder on a joint and publishes its position and velocity on the CAN
bus. Firmware lives in `src/abs`.

Absolute encoders keep their reading across a power cycle, so a joint knows where it is at boot
without a homing sequence. This is what the arm's differential and other joints use for position
feedback.

## Hardware

| Interface | Peripheral | Notes |
| --- | --- | --- |
| AS5047U magnetic encoder | SPI (DMA) | 14-bit absolute rotary position |
| CAN | FDCAN1 | |
| Logging | LPUART1 (DMA) | see [esw.stlink](../../reference/python/stlink.md) |

## Behavior

Two independent timers, both configured from flash at startup:

| Timer | Rate | Purpose |
| --- | --- | --- |
| `htim16` | `poll_frequency` | read the encoder |
| `htim17` | `publish_frequency` | send `ABSEncoderState` |

Both default to 30 Hz in the `.ioc` and are overwritten by the configured values during `init()`.
Decoupling them means you can poll fast for a smooth velocity estimate while publishing slowly to
keep bus load down.

The raw encoder reading is transformed before publishing, using the configured options:

- `invert` flips the direction of travel.
- `position_offset` is subtracted, defining where zero is.
- `output_scalar` scales the result, which is how a gear ratio is applied.
- `continuous_mode` selects whether the reading wraps at a full turn or accumulates across turns.
- `bounded_mode` clamps the output to `min_bound` and `max_bound`.

## CAN Interface

### Receives

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `ABSResetCmd` | `0x80210000` | 1 | `reset`, `clear_faults` |
| `ABSZeroCmd` | `0x80220000` | 4 | `offset` |
| `ESWConfigCmd` | `0x80F00000` | 6 | `address`, `value`, `apply` |
| `ESWProbe` | `0x80F10000` | 4 | `data` |

### Sends

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `ABSEncoderState` | `0x80200000` | 8 | `position` (32, signed), `velocity` (32, signed) |
| `ESWAck` | `0x80F20000` | 4 | `data` |

## Zeroing

`ABSZeroCmd` takes the encoder's current raw reading, writes it to the `position_offset` register
and applies it immediately. The joint is therefore zeroed **wherever it is standing when the
command arrives**, so move it to the reference position first.

Because `position_offset` is a configuration register, the new zero is written to flash and
survives a power cycle.

`tools/scripts/abs_zero.py` does this for the arm's differential pitch and roll encoders, but with
hardcoded CAN IDs (`0x37`, `0x38`) on `can1`. Read it before running it.

## Configuration

Definition: `config/abs.yaml`. Device values live under `rover/ra/`, for example
`rover/ra/abs_de_pitch.yaml`.

| Register | Type | Meaning |
| --- | --- | --- |
| `can_id` | `uint8` | this board's CAN node ID |
| `host_can_id` | `uint8` | where to address published state, normally `0x10` |
| `sys_cfg` | `uint16` | bit field: `continuous_mode` (0), `bounded_mode` (1), `invert` (2) |
| `output_scalar` | `float32` | applied to the raw reading, e.g. a gear ratio |
| `position_offset` | `float32` | subtracted from the raw reading; set by `ABSZeroCmd` |
| `poll_frequency` | `float32` | encoder read rate, Hz |
| `publish_frequency` | `float32` | CAN publish rate, Hz |
| `min_bound` | `float32` | lower clamp, used when `bounded_mode` is set |
| `max_bound` | `float32` | upper clamp, used when `bounded_mode` is set |

28 bytes total. See [Register Definitions](../../reference/config/schema.md) and
[CAN Configuration Interface](../../reference/config/can-interface.md).

## Building

```bash
./scripts/build.sh --src src/abs --preset Debug
./scripts/build.sh --src src/abs --preset Debug --flash
```

`src/abs` is in `ci.json`, so CI builds it at both presets.
