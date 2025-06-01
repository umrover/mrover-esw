# ESW 2025-2026 Projects

This list is not extensive. For more additional information about these projects or
other projects not listed here, please reach out to an ESW lead.

## BM Improved Configuration

**Status**: Not Started

Each [3BM](../info/brushed.md) needs to be configured to set up limit switches, encoders,
max voltage, etc. This configuration is stored in RAM which is volatile memory. This means
that every time the 3BM gets power cycled, it forgets its configuration, which requires us
to resend its configuration over CAN before we try to command a motor every time we turn
the board on. Furthermore, once the board is configured, there is no way to reconfigure
it without power cycling the board and resending a new configuration. This is inefficient
and inconvenient.

An alternative to this would be to store 3BM configuration values in flash which is
non-volatile memory. This means that the values will persist even through power cycles.

## BM Diagnostic GUI

**Status**: Not Started

There is currently no easy way to communicate over CAN with a 3BM without going through the
entire ROS2 stack. A simple GUI application which allows us to easily interface the 3BM
would massively improve the debugging experience. It would show data from the 3BM such as
its configuration and telemetry (motor status, velocity, etc). It would also feature a way
to send different commands. Ideally, it would be built using the Qt framework in C++. An
example of what the GUI could look like and feature would be the [tview](https://github.com/mjbots/moteus/blob/main/docs/getting_started.md#software)
application which we use to debug the moteus controllers for our brushless motors.

As a GUI application, this project will naturally require little hardware for testing much
of the code.

## Absolute Encoder Board

**Status**: In Progress (early)

Currently, the 3BM firmware uses I2C in order to read from an [AS5048B](https://ams-osram.com/products/sensor-solutions/position-sensors/ams-as5048b-high-resolution-position-sensor)
absolute encoder which is connected to the 3BM. In an attempt to reduce the complexity of
the 3BM firmware and increase modularity, we would like to create a separate board which
handles the absolute encoder reading and communicates with the 3BM over CAN. This board
would be a simple PCB with an STM32 microcontroller, an absolute encoder, and a CAN transceiver.
This would allow us to rapidly poll the absolute encoder (e.g. at 1 kHz) over I2C/SPI and
have the 3BM (or any other board) read the absolute encoder at a lower frequency (e.g. 100 Hz)
over CAN.

EHW has begun designing this board, but it is still in the early stages. The current hardware
is as follows:

**Components**:

- STM32G431K6T6 (microcontroller)
- AS5047U/AS5147U (absolute encoder)
- TCAN1044VDRQ1 (CAN transceiver)

**Architecture**:

- G431 <-- _SPI CS, CLK, MISO, MOSI_ --> AS5047U
- G431 <-- _CAN TX/RX_ --> TCAN1044VDRQ1
- G431 <-- _GPIO_ --> CAN TX/RX Status LEDs

## Mini Arm Controller

**Status**: Not Started

_Disclaimer: This project is one of the more complex ESW projects. However, it has
a lower priority as far as rover functionality goes. Because of this, this project
will be best suited for a group of rover members who would like to work on a rover-related
project for EECS 373 or 473 (probably best suited for 473)._

The robotic arm on our rover is controlled using an Xbox controller where each joystick, button,
and trigger corresponds to moving a specific joint on the arm. Another method that we propose
to control the arm is by having a miniature version of the arm which acts as a "voodoo puppet"
of the arm - however the mini arm is moved is how the actual arm on the rover will move.
At a basic level, this would simply require having rotational positional data on each joint of
the mini arm and these positions can be commanded to the rover arm. This project, however, can
get very complex when considering some edge cases. For example: what should happen if the rover
arm is unable to move (e.g. touches the ground). If we let the mini arm continue to move, then
we would be dangerously moving the arm. Handling cases like these will be an important discussion
to have before starting development of this project.

## CANalyzer

**Status**: In Progress

Please see the canalyzer branch in the mrover-esw repository. There is a README.md in `src/canalyzer`
