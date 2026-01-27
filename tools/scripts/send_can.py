from time import sleep

from esw import esw_logger
from esw.can.dbc import get_dbc
from esw.can.canbus import CANBus


def on_msg_recv(msg):
    msg_name, signals, src_id, dest_id = msg
    if msg_name != "BMCMotorState":
        # pass
        esw_logger.info(f"CAN RECV {msg_name} (src: {hex(src_id)}, dest: {hex(dest_id)}): {signals}")
    else:
        pass
        # esw_logger.info(f"CAN RECV {msg_name} (src: {src_id}, dest: {dest_id}): {signals}")


if __name__ == "__main__":
    NUM_LOOPS = 50
    LOOP_DELAY = 0.05
    TARGET = 0
    INC = 0.10
    with CANBus(get_dbc(dbc_name="CANBus1"), "can0", on_recv=on_msg_recv) as bus:
        # n = 0
        # while True:
        #     bus.send("BMCProbe", {"data": n}, node_id=0)
        #     bus.send("BMCProbe", {"data": n}, node_id=n)
        #     sleep(1)
        #     n += 1
        #     if n > 0xFF:
        #         n = 0
        # request can id, max pwm of board
        bus.send("BMCConfigCmd", {"address": 0x00, "value": 0x00, "apply": 0x0}, node_id=0)
        bus.send("BMCConfigCmd", {"address": 0x24, "value": 0x00, "apply": 0x0}, node_id=0)
        # set max pwm of board
        bus.send("BMCConfigCmd", {"address": 0x24, "value": 1.0, "apply": 0x1}, node_id=0, floats_to_uint32=True)
        # set motor en on board
        bus.send("BMCConfigCmd", {"address": 0x01, "value": 0b00000001, "apply": 0x1}, node_id=0)
        # request pwm, motor en of board
        bus.send("BMCConfigCmd", {"address": 0x24, "value": 0x00, "apply": 0x0}, node_id=0)
        bus.send("BMCConfigCmd", {"address": 0x01, "value": 0x00, "apply": 0x0}, node_id=0)
        # set mode to throttle
        bus.send("BMCModeCmd", {"mode": 5, "enable": 1}, node_id=0)
        while True:
            for _ in range(NUM_LOOPS):
                bus.send("BMCTargetCmd", {"target": TARGET, "target_valid": 1}, node_id=0)
                sleep(LOOP_DELAY)
            TARGET += INC
            if abs(TARGET - 1.0) < 0.01 or abs(TARGET + 1.0) < 0.01:
                INC *= -1

        # while True:
        #     for _ in range(NUM_LOOPS):
        #         bus.send("BMCTargetCmd", {"target": TARGET, "target_valid": 1}, node_id=0)
        #         sleep(LOOP_DELAY)
        #     for _ in range(NUM_LOOPS):
        #         bus.send("BMCTargetCmd", {"target": -TARGET, "target_valid": 1}, node_id=0)
        #         sleep(LOOP_DELAY)
        sleep(LOOP_DELAY)
