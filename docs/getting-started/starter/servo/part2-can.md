# Servo Starter Project - Part 2 - CAN

This starter project is made up of two parts: PWM and CAN. This is Part 2 - CAN.

By the end of this part, you should have an understanding of the CAN (controller area network)
communication protocol. 

Just as a reminder, if you have any questions, feel free to reach out to any
ESW lead or member. This project isn't meant to be high-stakes,
so please reach out if you ever get stuck!

## Prerequisites

* STM32CubeIDE [installed](../../stm32cubeide/index.md)
* LED Starter Project [completed](https://github.com/umrover/embedded-testbench/wiki/Nucleo-LED-Starter-Project) and shown to an ESW lead
* Servo Part 1 Starter Project [completed](https://github.com/umrover/embedded-testbench/wiki/Nucleo-LED-Starter-Project) and shown to an ESW lead
* STM32F303RE Nucleo
* MCP2551 CAN tranceiver
* 220&Omega; resistor
* Jumper cables

## Guide

### 1. Setting up the project
Since you already have practice creating a project, you will only need to clone and open the
premade STM32 project for this starter project.

Go to [this repository](https://github.com/umrover/mrover-esw), click the "Code" tab, and copy the SSH URL.
![opened "Code" tab with boxes showing how to copy the SSH URL]()

You now have the URL you need to clone the project.

Clone the project onto your local computer by running the following command in your terminal:
```sh
git clone <URL copied in above step>
```

Enter the directory:
```sh
cd mrover-esw
```

Then, create a new branch for yourself
```sh
git checkout -b <starter/your-name>
```

Open STM32CubeIDE and open the Servo ***Part 1*** starter project (the directory named `p2-can`).


