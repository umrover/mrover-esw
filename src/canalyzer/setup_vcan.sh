#!/bin/bash
# vibe coded vcan creation

# Check if the interface already exists
if ! ip link show vcan0 &> /dev/null; then
    sudo ip link add dev vcan0 type vcan
    sudo ip link set up vcan0
    echo "vcan0 created and set up."
else
    echo "vcan0 already exists."
fi
if ! ip link show vcan1 &> /dev/null; then
    sudo ip link add dev vcan1 type vcan
    sudo ip link set up vcan1
    echo "vcan1 created and set up."
else
    echo "vcan1 already exists."
fi
if ! ip link show vcan1 &> /dev/null; then
    sudo ip link add dev vcan1 type vcan
    sudo ip link set up vcan1
    echo "vcan1 created and set up."
else
    echo "vcan1 already exists."
fi
if ! ip link show vcan1 &> /dev/null; then
    sudo ip link add dev vcan1 type vcan
    sudo ip link set up vcan1
    echo "vcan1 created and set up."
else
    echo "vcan1 already exists."
fi
