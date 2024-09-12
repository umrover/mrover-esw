# Servo Starter Project - Part 2 - CAN

This starter project is made up of two parts: PWM and CAN. This is Part 2 - CAN.

By the end of this part, you should have an understanding of the CAN (controller area network)
communication protocol.

Just as a reminder, if you have any questions, feel free to reach out to any
ESW lead or member. This project isn't meant to be high-stakes,
so please reach out if you ever get stuck!

## Prerequisites

* STM32CubeIDE [installed](../../stm32cubeide/index.md)
* LED Starter Project [completed](../led/index.md) and shown to an ESW lead
* Servo Part 1 Starter Project [completed](part1-pwm.md) and shown to an ESW lead
* STM32G431RB Nucleo
* MCP2551 CAN tranceiver

***Before starting this project***, please give the [CAN info page](../../../info/communication-protocols/index.md#can)
a read. It will get you start on the basics of CAN communication. ***Do not read "Using the Kvaser Bit Timing Calculator."***

## Guide

### 1. Setting up the project
Assuming you have already completed the Servo Part 1 Starter Project, you will not have to clone
the project again.

Open STM32CubeIDE and open the Servo ***Part 2*** starter project (the directory named `p2-can`).

### 2. CAN configuration

Once the .ioc is open, activate the FDCAN peripheral. You can do this by clicking on the FDCAN
peripheral in the `Connectivity` tab on the left side of the .ioc. Then, check the `Activated` box.

![fdcan config window](fdcan-config.webp)

Next, we need to configure the timings for the CAN bus. Click on the `Parameter Settings` tab
(as shown above). We will be using CAN 2.0B *not* CAN FD, so `Frame Format` should be set to
"Classic." Since we are using "Classic," we do not need to worry about the data bit rate. You will
only need to properly set the following values:

1. Nominal Prescaler
2. Nominal Sync Jump Width
3. Nominal Time Seg 1
4. Nominal Time Seg 2

There is a very in depth guide to configuring these values for STM32 MCUs in the
[CAN FD Bit Timing](../../../info/communication-protocols/index.md#can-bit-timing) section of the
docs. However, for the purposes of this starter project, you can use [this online calculator](https://phryniszak.github.io/stm32g-fdcan/)
designed for STM32G4 boards. Our FDCAN peripheral is clocked at 72 MHz, and the CAN bus we are
trying to communicate on is running at 500 kbps. The calculator will give you the values you need
to input into the .ioc. ***Ignore all CAN FD related values in the calculator.*** For Nominal Sync
Jump Width, make sure to read the note at the bottom of the calculator.

After inputting the bit timing parameters, ensure that `Nominal Baud Rate` is 500000 bit/s.
You can now save the .ioc and generate the code.

### 3. Writing the CAN code

In the `main.c`, you should see 6 TODOs. We will walk through each of them.

#### TODO 1: Create the necessary variables to send a CAN message
In order to send a CAN message, we need to create an object of type `FDCAN_TxHeaderTypeDef`. This
will store information about the CAN message we are sending, such as the identifier, the data
length, the frame format, etc.

#### TODO 2: Start the CAN peripheral

#### TODO 3: Form the CAN message header

#### TODO 4-6: Send a CAN message to rotate the servo


