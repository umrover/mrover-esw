# Limit Switch Board (LIM)

Reports the state of two limit switches on the CAN bus. Firmware lives in `src/lim`.

This is the simplest board in the tree. It exists for places that need limit switches without a
motor controller attached to read them. The mast limit is the current user.

## Hardware

| Interface | Peripheral | Notes |
| --- | --- | --- |
| Limit switches A and B | GPIO | presence and polarity are configurable |
| CAN | FDCAN1 | |
| Logging | LPUART1 (DMA) | |

## Behavior

| Timer | Rate | Purpose |
| --- | --- | --- |
| `htim6` | 10 Hz | publish `LIMState` |

There is no state machine and nothing to calibrate. The board reads both pins and publishes them
on a fixed cadence. Configuration decides whether each switch is present at all, whether it is
enabled, and whether it reads active high or active low.

## CAN Interface

### Receives

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `LIMResetCmd` | `0x80310000` | 1 | `reset`, `clear_faults` |
| `ESWConfigCmd` | `0x80F00000` | 6 | `address`, `value`, `apply` |
| `ESWProbe` | `0x80F10000` | 4 | `data` |

### Sends

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `LIMState` | `0x80300000` | 1 | `lim_a` (1), `lim_b` (1) |
| `ESWAck` | `0x80F20000` | 4 | `data` |

## Configuration

Definition: `config/lim.yaml`, 8 bytes, the smallest in the tree. Device values live under
`rover/cm/`, for example `rover/cm/mast_limit.yaml`.

| Register | Type | Meaning |
| --- | --- | --- |
| `can_id` | `uint8` | this board's CAN node ID |
| `host_can_id` | `uint8` | where to address published state |
| `sys_cfg` | `uint8` | `lim_a_present` (bit 0), `lim_b_present` (bit 1) |
| `limit_cfg` | `uint8` | `lim_a_en` (0), `lim_a_active_high` (1), `lim_b_en` (2), `lim_b_active_high` (3) |
| `publish_frequency` | `float32` | CAN publish rate, Hz |

See [Register Definitions](../../reference/config/schema.md) and
[CAN Configuration Interface](../../reference/config/can-interface.md).

!!! warning
    `publish_frequency` is **not currently read by the firmware**. The transmit rate is whatever
    `htim6` is set to in the `.ioc`, currently 10 Hz. Writing this register has no effect.

## Building

```bash
./scripts/build.sh --src src/lim --preset Debug
./scripts/build.sh --src src/lim --preset Debug --flash
```

!!! warning
    `src/lim` is **not** in `ci.json`, so it is not built by CI. Build it locally before pushing.
