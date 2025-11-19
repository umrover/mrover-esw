#!/bin/bash

# Compile if needed
if [ -e ../can_code/test.out ]; then
    echo "Test exists"
else 
    g++ -Wall -Werror ../can_code/test.cpp -o ../can_code/test.out
fi

# Run test.out in the background
../can_code/test.out &
TEST_PID=$!

# Run setup_vibe.sh in the background
./setup_vibe.sh &
VIBE_PID=$!

# Let them run for 10 seconds
sleep 1

# Kill both programs
kill $TEST_PID
kill $VIBE_PID
echo "Programs terminated after 10 seconds"
sleep 1

# Continue with CAN tools
canplayer -I ../can_code/can0.log
candump vcan0
