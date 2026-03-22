# Dynamixel Wizard

Dynamixel Wizard 2.0 is a GUI tool from ROBOTIS for configuring and debugging Dynamixel servos over a serial interface (e.g., U2D2).

---

## Purpose

Used for:
- detecting servos on a bus
- reading/writing control table values
- setting ID and baud rate
- setting operating mode
- enabling/disabling torque
- verifying telemetry (position, velocity, current, voltage)

Not used for runtime rover control.

---

## Hardware Setup

Computer -> U2D2 -> Dynamixel Bus -> Servo
|> External Power Supply


The USB adapter provides communication only. The servo must be powered separately.

Requirements:
- U2D2 (or equivalent USB-to-Dynamixel adapter)
- correct voltage supply
- correct cabling

---

## Basic Usage

To connect to a servo:

1. Select the correct serial port  
2. Set the baud rate  
3. Click **Scan**  
4. Select the detected servo  

If the servo does not appear, the issue is almost always power, wiring, or baud rate mismatch.

---

## Key Configuration

### ID and Baud Rate
Each servo on a shared bus must have a unique ID. The baud rate must match what rover software expects. A mismatch in either will make the servo appear disconnected.

### Operating Mode
Servos support multiple modes (position, velocity, current, etc.). The selected mode must match how the servo is used in code. Incorrect modes lead to unexpected or no movement.

### Torque Enable
Torque must be enabled for motion. Many configuration values require torque to be disabled before they can be changed.

---

## Setup Procedure

1. Connect only one servo  
2. Scan and confirm detection  
3. Record current settings  
4. Set ID and baud rate  
5. Set operating mode  
6. Power cycle if needed  
7. Verify telemetry updates  
8. Test small movements  

---

## Verifying Operation

A working servo should show:
- position updating when moved  
- nonzero current under load  
- stable voltage  
- reasonable temperature (< ~70°C)  

---

## Reset and Firmware

Factory reset can be used if configuration is unknown or incorrect. After resetting, all parameters must be reconfigured.

Firmware updates should only be performed when necessary and require stable power. Interrupting an update can permanently damage the device.

---

## Troubleshooting

### Servo not detected
- no power  
- incorrect port  
- incorrect baud rate  
- bad cable  
- port in use  

Try:
- different baud rates  
- reconnecting cables  
- power cycling  
- testing a single servo  

---

### Servo detected but rover code fails
- ID mismatch  
- baud mismatch  
- wrong port in code  
- protocol mismatch  

If Wizard works, the hardware is functioning and the issue is in software or configuration.

---

### Servo does not move
- torque disabled  
- incorrect operating mode  
- limits too restrictive  
- command not updating  

---

### Servo weak or unstable
- current limit too low  
- incorrect gains  
- wrong operating mode  

---

### Servo overheating or erratic
- excessive load  
- high current draw  
- incorrect limits  
- repeated stalls  

Stop testing if overheating occurs.

---

## Notes

- Configure servos individually before using a shared bus  
- Always match Wizard configuration with rover software  
- Most failures are configuration issues, not hardware faults  
