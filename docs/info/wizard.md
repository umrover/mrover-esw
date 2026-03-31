# Dynamixel Wizard 2.0

DYNAMIXEL Wizard 2.0 is a GUI tool from ROBOTIS for configuring and debugging Dynamixel servos over a serial interface. It is used for bench setup and diagnostics — not runtime control.

Download: [DYNAMIXEL Wizard 2.0](https://emanual.robotis.com/docs/en/software/dynamixel/dynamixel_wizard2/)

## Purpose

Used for detecting servos on a bus, reading/writing control table values, setting ID and baud rate, configuring operating mode, enabling/disabling torque, and verifying telemetry (position, velocity, current, voltage). Not used for runtime control.

## Hardware Setup

```
Computer --> U2D2 --> Dynamixel Bus --> Servo(s)
                                    |
                          External Power Supply
```

The U2D2 carries data only. Servos must be powered externally — correct voltage for the servo model (5V for XL330).

## Connecting to a Servo

1. Select the correct serial port (`/dev/ttyUSB0` on Linux, `COM*` on Windows)
2. Set the baud rate (default 57600 for X series)
3. Click **Scan**
4. Select the detected servo from the list

If a servo does not appear, the cause is almost always power, wiring, or a baud rate mismatch. Try scanning all baud rates, power cycling, and isolating to a single servo.

## Key Configuration

### ID and Baud Rate

Every servo on a shared bus must have a unique ID (1–252). The baud rate must match what your software expects. Either mismatch will make the servo appear disconnected. Configure servos one at a time before chaining them on the bus.

### Operating Mode

| Mode Value | Name | Description |
|---|---|---|
| 1 | Velocity Control | Continuous rotation at a target speed |
| 3 | Position Control | Move to an absolute position (0–4095) |
| 4 | Extended Position | Multi-turn position control |
| 5 | Current-Based Position | Position control with current limiting |
| 16 | PWM Control | Direct PWM duty cycle |

The operating mode set in Wizard must match how the servo is used in code. Mismatched modes cause unexpected or no movement.

### Torque Enable

Torque must be enabled for motion. Most EEPROM configuration values (ID, baud rate, operating mode) require torque to be **disabled** before they can be changed. Wizard handles this automatically when editing those fields, but be aware of it when writing values manually via the control table.

## Setup Procedure

1. Connect only one servo
2. Scan and confirm detection
3. Record current settings
4. Set ID and baud rate
5. Set operating mode
6. Power cycle if required
7. Verify telemetry is updating
8. Test small movements with the goal position slider

## Verifying Operation

A healthy servo should show:
- Present Position updating when the shaft is moved manually (torque off)
- Nonzero Present Current under load
- Stable bus voltage within spec
- Temperature below ~70°C under sustained load

## Reset and Firmware

**Factory Reset** wipes the control table back to factory defaults. Use it when configuration is unknown or corrupted — all parameters must be reconfigured afterward. ID and baud rate will return to defaults (ID 1, 57600).

**Firmware Update** should only be performed when necessary. Requires stable power throughout — interrupting an update can permanently brick the device.

## Troubleshooting

### Servo not detected

Check power, port selection, baud rate, and cabling first. Then try:
- Scanning all baud rates
- Reconnecting all cables and verifying common ground
- Power cycling the servo
- Isolating to a single servo on the bus

### Servo detected in Wizard but rover code fails

The hardware is working — the issue is in software configuration. Check for ID mismatch, baud rate mismatch, wrong port in code, or protocol version mismatch (Protocol 1.0 vs 2.0).

### Servo does not move

- Torque not enabled
- Operating mode does not match the command type being sent
- Position or velocity limits too restrictive
- Goal position command not actually reaching the servo

### Servo weak or unstable

- Current limit set too low
- PID gains need tuning
- Wrong operating mode for the application

### Servo overheating or behaving erratically

- Excessive mechanical load
- Current limits too high for the thermal environment
- Repeated stalls without rest time

Stop testing immediately if overheating occurs. Allow the servo to cool before resuming.

## Notes

- Configure servos individually before placing them on a shared bus
- Wizard configuration must match rover software (ID, baud rate, operating mode, protocol version)
- Most failures are configuration issues, not hardware faults
