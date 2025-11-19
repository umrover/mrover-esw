#!/bin/bash


ip link set vcan0 down 2>/dev/null
ip link set vcan1 down 2>/dev/null
ip link set vcan2 down 2>/dev/null
ip link set vcan3 down 2>/dev/null

ip link delete vcan0 type vcan 2>/dev/null
ip link delete vcan1 type vcan 2>/dev/null
ip link delete vcan2 type vcan 2>/dev/null
ip link delete vcan3 type vcan 2>/dev/null
