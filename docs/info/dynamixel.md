# Dynamixel Servo Wiki

## Hardware: XL330-M288-T

The [XL330-M288-T](https://www.robotis.us/dynamixel-xl330-m288-t/) is a compact TTL servo from the XL330 family.

| Parameter | Value |
|---|---|
| Operating Voltage | 3.7 – 6.0 V (5.0 V recommended) |
| Stall Torque | 0.52 N·m @ 5V |
| No-load Speed | 61 RPM @ 5V |
| Resolution | 4096 positions (0.088°/step) |
| Protocol | Dynamixel Protocol 2.0 |
| Default Baud Rate | 57600 bps |
| Default ID | 1 |
| Communication | TTL half-duplex serial |
| Connector | JST 3-pin (TTL) |

## Interface Hardware: U2D2

The [U2D2](https://www.robotis.us/u2d2/) converts USB to TTL or RS-485 half-duplex serial (`/dev/ttyUSB0` on Linux). It carries data only — servo bus power must come from an external supply. The **U2D2 Power Hub** accessory provides regulated 5V or 12V to the bus from a bench supply.

## Protocol 2.0 Packet Structure

```
Header         | Reserved | Packet ID | Length  | Instruction | Parameters | CRC
0xFF 0xFF 0xFD | 0x00     | 0x00–0xFE | 2B (LE) | 1 byte      | N bytes    | 2B (LE)
```

- **Packet ID** — target servo ID (0–252), or `0xFE` for broadcast
- **Length** — byte count from Instruction through CRC
- **CRC** — 16-bit CRC over Packet ID onward

### Instruction Set

| Instruction | Value | Description |
|---|---|---|
| `PING` | `0x01` | Check presence; returns model number |
| `READ` | `0x02` | Read N bytes from a control table address |
| `WRITE` | `0x03` | Write data to a control table address |
| `REG_WRITE` | `0x04` | Stage a write; executes on ACTION |
| `ACTION` | `0x05` | Execute all pending REG_WRITE commands |
| `FACTORY_RESET` | `0x06` | Reset control table to factory defaults |
| `REBOOT` | `0x08` | Reboot the servo |
| `CLEAR` | `0x10` | Clear multi-turn info or error states |
| `SYNC_READ` | `0x82` | Read the same address from multiple servos |
| `SYNC_WRITE` | `0x83` | Write to the same address on multiple servos |
| `BULK_READ` | `0x92` | Read different addresses from multiple servos |
| `BULK_WRITE` | `0x93` | Write different addresses to multiple servos |

### Status Packet

After every non-broadcast instruction the servo responds:

```
0xFF 0xFF 0xFD | 0x00 | ID | Length | 0x55 | Error | Parameters | CRC
```

The **Error** byte contains alert and hardware error flags — check it to detect overheating, overload, or electrical faults.

### Key Control Table Addresses (X Series / XL330)

| Address | Size | Name | Description |
|---|---|---|---|
| 0 | 2 | Model Number | Read-only |
| 7 | 1 | Firmware Version | Read-only |
| 11 | 1 | Operating Mode | 1=Vel, 3=Position, 4=ExtPos, 5=CurrPos, 16=PWM |
| 64 | 1 | Torque Enable | 0=off, 1=on |
| 116 | 4 | Goal Position | Target position (0–4095) |
| 132 | 4 | Present Position | Current position (read-only) |
| 126 | 4 | Goal Velocity | Target speed in velocity mode |
| 128 | 2 | Goal PWM | Direct PWM control |
| 146 | 2 | Present Velocity | Read-only |
| 144 | 2 | Present Current | mA, read-only |

> **Note:** Always disable torque (address 64 = 0) before changing Operating Mode or any EEPROM settings.

## DynamixelSDK (ROS 2 / C++)

The [DynamixelSDK `humble` branch](https://github.com/ROBOTIS-GIT/DynamixelSDK/tree/humble) targets ROS 2 Humble.

```bash
sudo apt install ros-humble-dynamixel-sdk
```

The SDK exposes two key objects: `PortHandler` (serial port) and `PacketHandler` (Protocol 2.0 framing). Read/write functions are selected by register width — `write1ByteTxRx`, `write2ByteTxRx`, `write4ByteTxRx` and their read counterparts.

### ROS 2 Node Example

The SDK ships `read_write_node.cpp` — a minimal node that subscribes to `/set_position` and serves `/get_position`. The key setup pattern, common to any node:

```cpp
// Control table addresses (X series / XL330)
#define ADDR_OPERATING_MODE    11
#define ADDR_TORQUE_ENABLE     64
#define ADDR_GOAL_POSITION    116
#define ADDR_PRESENT_POSITION 132

#define PROTOCOL_VERSION  2.0
#define BAUDRATE          57600
#define DEVICE_NAME       "/dev/ttyUSB0"

// Init port and packet handlers before rclcpp::init()
portHandler   = dynamixel::PortHandler::getPortHandler(DEVICE_NAME);
packetHandler = dynamixel::PacketHandler::getPacketHandler(PROTOCOL_VERSION);
portHandler->openPort();
portHandler->setBaudRate(BAUDRATE);

// Set position control mode and enable torque (broadcast to all IDs)
packetHandler->write1ByteTxRx(portHandler, BROADCAST_ID, ADDR_OPERATING_MODE, 3, &dxl_error);
packetHandler->write1ByteTxRx(portHandler, BROADCAST_ID, ADDR_TORQUE_ENABLE,  1, &dxl_error);

// ... spin node ...

// Disable torque on shutdown
packetHandler->write1ByteTxRx(portHandler, BROADCAST_ID, ADDR_TORQUE_ENABLE, 0, &dxl_error);
```

Inside the node, writing a goal position and reading present position:

```cpp
// Set goal position (4 bytes for X series)
packetHandler->write4ByteTxRx(portHandler, dxl_id, ADDR_GOAL_POSITION, goal_position, &dxl_error);

// Read present position
packetHandler->read4ByteTxRx(portHandler, dxl_id, ADDR_PRESENT_POSITION,
                              reinterpret_cast<uint32_t*>(&present_position), &dxl_error);
```

> **Note for AX / MX(1.0) series:** Goal/Present Position is 2 bytes — use `write2ByteTxRx` / `read2ByteTxRx`.

## Common Gotchas

**Torque must be disabled to change Operating Mode.** Writing to address 11 while torque is on returns `COMM_SUCCESS` but the servo silently ignores the write.

**Broadcast writes do not generate a status packet.** When using `BROADCAST_ID` (`0xFE`), no response is returned — this is by Protocol 2.0 design. Do not wait for a reply.

**CRC errors usually mean a wiring issue.** Frequent `COMM_RX_CORRUPT` errors point to a loose cable, missing common ground, or an overly long bus.

**Default baud rate is 57600.** Your SDK `BAUDRATE` define must match address 8 in the servo's control table. Use DYNAMIXEL Wizard 2.0 to change it if needed.

**The U2D2 does not supply bus power.** Use a 5V external supply (for XL330) via the Power Hub or your own wiring.

## Other Resources

[DYNAMIXEL Protocol 2.0 eManual](https://emanual.robotis.com/docs/en/dxl/protocol2/)\
[XL330-M288-T eManual](https://emanual.robotis.com/docs/en/dxl/x/xl330-m288/)\
[U2D2 eManual](https://emanual.robotis.com/docs/en/parts/interface/u2d2/)\
[DynamixelSDK GitHub (humble branch)](https://github.com/ROBOTIS-GIT/DynamixelSDK/tree/humble)\
[DYNAMIXEL Wizard 2.0](https://emanual.robotis.com/docs/en/software/dynamixel/dynamixel_wizard2/) — GUI tool for scanning, configuring, and testing servos
