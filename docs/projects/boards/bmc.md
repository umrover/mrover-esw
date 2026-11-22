# Brushed Motor Controller (BMC)

Drives a brushed DC motor with closed-loop position or velocity control, quadrature encoder
feedback, limit switches and current sensing. Firmware lives in `src/bmc`.

This is the most complex board in the tree and the only one that runs a control loop. For
background on the motors themselves see [Brushed DC Motors](../../info/brushed.md).

## Hardware

| Interface | Peripheral | Notes |
| --- | --- | --- |
| H-bridge | `htim1` PWM + direction GPIO | motor output |
| Quadrature encoder | `htim4` in encoder mode | position and velocity feedback |
| Limit switches A and B | GPIO | presence and polarity are configurable |
| Current sense | ADC1 via AD8418A | published as `current` |
| CAN | FDCAN1 | |
| Logging | LPUART1 (DMA) | |

## Behavior

| Timer | Rate | Purpose |
| --- | --- | --- |
| `htim17` | 25 Hz | control loop update |
| `htim6` | 10 Hz | publish `BMCMotorState` |
| `htim16` | 10 Hz | CAN watchdog |
| `htim2` | - | elapsed-time source for PID and encoder timing |

### Modes

The board is a small state machine, selected by `BMCModeCmd`:

| Mode | Value | Behavior |
| --- | --- | --- |
| `STOPPED` | 0 | output disabled |
| `FAULT` | 1 | latched error; output disabled until reset |
| `THROTTLE` | 5 | open loop, target is a duty cycle |
| `POSITION` | 6 | closed loop on encoder position |
| `VELOCITY` | 7 | closed loop on encoder velocity |

Position and velocity modes each load their own PIDF gains from configuration when entered, so the
two loops are tuned independently.

### Watchdog

A CAN watchdog runs at 10 Hz. If commands stop arriving the board faults with `WWDG_EXPIRED` and
disables the output, so a lost link or a crashed host stops the motor rather than leaving it
running at its last command.

### Calibration

Position control requires a known reference. The board tracks an uncalibrated position until a
limit switch establishes the offset; commanding position or velocity before that faults with
`UNCALIBRATED`. Each limit switch can be configured as forward or backward, and can optionally
readjust the position estimate when hit.

### Fault Codes

Published in `BMCMotorState.fault_code`:

| Code | Meaning |
| --- | --- |
| `NONE` | no error |
| `NO_MODE` | no mode selected |
| `INVALID_CONFIGURATION_FOR_MODE` | closed-loop mode requested with no feedback configured |
| `INVALID_FLASH_CONFIG` | configuration in flash is not usable |
| `WWDG_EXPIRED` | CAN watchdog timed out |
| `UNCALIBRATED` | position or velocity commanded before calibration |
| `CAN_ERROR_FATAL` | unrecoverable CAN error |
| `I2C_ERROR_FATAL` | unrecoverable I2C error |
| `SPI_ERROR_FATAL` | unrecoverable SPI error |

Clear a latched fault with `BMCResetCmd`.

## CAN Interface

### Receives

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `BMCModeCmd` | `0x80100000` | 2 | `mode` (8), `enable` (1) |
| `BMCTargetCmd` | `0x80110000` | 5 | `target` (32, signed), `target_valid` (1) |
| `BMCResetCmd` | `0x80120000` | 1 | `reset`, `clear_faults` |
| `ESWConfigCmd` | `0x80F00000` | 6 | `address`, `value`, `apply` |
| `ESWProbe` | `0x80F10000` | 4 | `data` |

### Sends

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `BMCMotorState` | `0x80130000` | 16 | `mode` (8), `fault_code` (8), `position` (32, signed), `velocity` (32, signed), `current` (32, signed), `limit_a` (1), `limit_b` (1), `is_stalled` (1) |
| `ESWAck` | `0x80F20000` | 4 | `data` |

`target` is interpreted according to the active mode: duty cycle in throttle, position in position
mode, velocity in velocity mode. `target_valid` must be set for the command to take effect.

## Configuration

Definition: `config/bmc.yaml`, the largest in the tree at 84 bytes. Device values live under
`rover/ra/`, `rover/sp/`, `rover/cm/` and `rover/test/`.

| Register | Type | Meaning |
| --- | --- | --- |
| `can_id` | `uint8` | this board's CAN node ID |
| `host_can_id` | `uint8` | where to address published state |
| `sys_cfg` | `uint8` | `motor_en`, `motor_inv`, `quad_en`, `quad_phase`, `stall_en`, `lim_a_present`, `lim_b_present` |
| `limit_cfg` | `uint8` | per switch: `en`, `active_high`, `is_forward`, `use_readjust` for A (bits 0-3) and B (bits 4-7) |
| `quad_cpr` | `float32` | encoder counts per revolution |
| `gear_ratio` | `float32` | motor to output gear ratio |
| `rotor_output_ratio` | `float32` | rotor to output ratio |
| `limit_a_position` | `float32` | position asserted when limit A is hit |
| `limit_b_position` | `float32` | position asserted when limit B is hit |
| `max_pwm` | `float32` | output ceiling |
| `min_pos`, `max_pos` | `float32` | software position limits |
| `min_vel`, `max_vel` | `float32` | software velocity limits |
| `pos_k_p`, `pos_k_i`, `pos_k_d`, `pos_k_f` | `float32` | position loop PIDF gains |
| `vel_k_p`, `vel_k_i`, `vel_k_d`, `vel_k_f` | `float32` | velocity loop PIDF gains |
| `stall_current` | `float32` | current above which `is_stalled` is asserted |
| `delta_position` | `float32` | position change threshold used in stall detection |

See [Register Definitions](../../reference/config/schema.md) and
[CAN Configuration Interface](../../reference/config/can-interface.md).

## Building

```bash
./scripts/build.sh --src src/bmc --preset Debug
./scripts/build.sh --src src/bmc --preset Debug --flash
```

`src/bmc` is in `ci.json`, so CI builds it at both presets.

## Related Scripts

Both have hardcoded CAN IDs and interfaces, so read them before running:

- `tools/scripts/restart_bmc.py` sends `BMCResetCmd` to node 49 on `can1`.
- `tools/scripts/send_can.py` drives node `0x67` on `can0` to a fixed target at 20 Hz.
