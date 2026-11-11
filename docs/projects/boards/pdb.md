# Power Distribution Board (PDB)

Drives the rover's autonomy status LED and the CAN bus activity indicators. Firmware lives in
`src/pdb`.

The autonomy LED is the externally visible signal of what the rover is doing, which competition
rules require during autonomous navigation. The board sets an RGB LED to a commanded color and can
blink it.

## Hardware

| Interface | Peripheral | Notes |
| --- | --- | --- |
| Autonomy LED | three GPIO pins (R, G, B) | one pin per channel, so eight colors |
| CAN TX indicator | GPIO | pulses on transmit |
| CAN RX indicator | GPIO | pulses on receive |
| CAN | FDCAN1 | |

## Behavior

| Timer | Rate | Purpose |
| --- | --- | --- |
| `htim2` | 200 ms | LED blink toggle |

The LED is set directly from `PDBAutonLEDCmd`. When `blinking` is set the blink timer is started
and the LED toggles every 200 ms; when it is clear the timer is stopped and its counter reset, so
the LED returns to steady immediately rather than finishing a half period.

Because each channel is a single GPIO rather than a PWM output, only fully on or fully off per
channel is available, giving eight colors including off rather than an arbitrary RGB value.

## CAN Interface

### Receives

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `PDBAutonLEDCmd` | `0x80700000` | 1 | `red` (1), `green` (1), `blue` (1), `blinking` (1) |
| `PDBResetCmd` | `0x80710000` | 1 | `reset`, `clear_faults` |

### Sends

Nothing. The board is command-only and publishes no state.

!!! note
    PDB implements neither `ESWConfigCmd` nor `ESWProbe`, so it cannot be probed for liveness or
    configured over CAN the way ABS, BMC and LIM can.

## Configuration

Definition: `config/pdb.yaml`.

| Register | Type | Meaning |
| --- | --- | --- |
| `can_id` | `uint8` | this board's CAN node ID |
| `jetson_can_id` | `uint8` | the Jetson's CAN node ID |

The `can_filtering` block sets up an FDCAN receive filter on `can_id` (extended IDs) and rejects
all non-matching frames.

!!! warning
    PDB does not handle `ESWConfigCmd` yet (see the note above), so these registers cannot be
    written over CAN.

## Building

```bash
./scripts/build.sh --src src/pdb --preset Debug
./scripts/build.sh --src src/pdb --preset Debug --flash
```

`src/pdb` is in `ci.json`, so CI builds it at both presets.
