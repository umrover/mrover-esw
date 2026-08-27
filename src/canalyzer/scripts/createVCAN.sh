#!/bin/bash

# Ensure the virtual CAN kernel module is loaded
sudo modprobe vcan

# Loop through numbers 0 to 3 to create and configure the interfaces
for i in {0..3}; do
    sudo ip link add dev vcan$i type vcan
    sudo ip link set up vcan$i
    echo "vcan$i is up and running."
done