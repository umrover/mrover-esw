# Science Board

Reads the atmospheric sensors on the science payload and publishes their readings on the CAN bus.
Firmware lives in `src/science`.

## Hardware

Five sensors. Four sit on one I2C bus; the UV sensor is read through the ADC.

| Sensor | Part | Bus | Address | Publishes |
| --- | --- | --- | --- | --- |
| Temperature, humidity, pressure | BME280 | I2C | `0x77` | `temperature`, `humidity`, `pressure` |
| Carbon dioxide | SCD4x | I2C | `0x29` | `co2` |
| Oxygen | | I2C | `0x70` | `oxygen` |
| Ozone | | I2C | `0x73` | `ozone` |
| UV | analog | ADC | | `uv_index` |

## Behavior

| Timer | Rate | Purpose |
| --- | --- | --- |
| `htim2` | 400 ms | CO2 sensor transmit |
| `htim3` | 100 ms | CO2 sensor receive |
| `htim6` | 500 ms | publish `SCISensorData` |
| `htim7` | 500 ms | I2C watchdog |
| `htim15` | 100 ms | publish `SCISensorState` |
| `htim16` | 1000 ms | retry faulted sensors |

Every sensor derives from `ScienceSensor` (`src/science/Inc/ScienceSensor.hpp`), which gives them
all the same four-part interface:

| Method | Purpose |
| --- | --- |
| `init()` | configure the device; returns false if it does not respond |
| `poll()` | start an acquisition |
| `update()` | consume the result and store it |
| `restart()` | re-run `init()` on a faulted sensor and clear the fault if it succeeds |

Acquisition is asynchronous: `poll()` starts a transfer and the completion interrupt drives
`update()`, so the main loop never blocks on a slow sensor. The CO2 sensor gets its own transmit
and receive timers because its exchange is slower than the others.

### Fault Handling

Each sensor carries a state flag. One that fails to respond is flagged and the board keeps running
with the rest, publishing the flags so the rover knows which readings to trust. A 1 Hz timer
retries flagged sensors through `restart()`, so a sensor that browns out or is reseated recovers on
its own without a board reset.

## CAN Interface

The board publishes to host `0x10` from node `0x40`, both fixed in
`src/science/Inc/config.hpp`.

### Receives

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `SCIResetCommand` | `0x80510000` | 1 | `reset`, `clear_faults` |

### Sends

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| `SCISensorData` | `0x80600000` | 32 | `uv_index`, `temperature`, `humidity`, `pressure`, `oxygen`, `ozone`, `co2` (seven 32-bit signed values) |
| `SCISensorState` | `0x80610000` | 1 | `uv_state`, `thp_state`, `oxygen_state`, `ozone_state`, `co2_state` (one bit each) |

`clear_faults` clears the sensor state flags without restarting the board; `reset` restarts the
MCU.

!!! note
    Science implements neither `ESWConfigCmd` nor `ESWProbe`, so it cannot be probed or configured
    over CAN.

## Configuration

None over CAN. There is no `config/science.yaml`; the CAN IDs are compile-time constants in
`src/science/Inc/config.hpp`, so changing them means rebuilding and reflashing.

## I2C Notes

The BME280 needs its factory calibration coefficients read out of NVM at startup before any reading
can be converted to physical units, which is why `init()` does several blocking reads before the
sensor becomes usable. The CO2 sensor has CRC checking disabled during initialization.

See [Communication Protocols](../../info/communication-protocols/index.md#i2c) for background on
I2C itself.

## Building

```bash
./scripts/build.sh --src src/science --preset Debug
./scripts/build.sh --src src/science --preset Debug --flash
```

!!! warning
    `src/science` is **not** in `ci.json`, so it is not built by CI. Build it locally before
    pushing.
