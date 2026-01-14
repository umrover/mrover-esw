from time import sleep

from esw import esw_logger
from esw.can.dbc import get_dbc
from esw.can.canbus import CANBus


def on_msg_recv(msg):
    msg_name, signals, node_id = msg
    if msg_name != "BMCMotorState":
        esw_logger.info(f"CAN RECV {msg_name} (Node {hex(node_id)}): {signals}")
    else:
        # pass
        esw_logger.info(f"CAN RECV {signals})")


if __name__ == "__main__":
    with CANBus(get_dbc(dbc_name="CANBus1"), "vcan0", on_recv=on_msg_recv) as bus:
        n = 0
        while True:
            bus.send("BMCProbe", {"data": n}, node_id=0)
            sleep(1)
            n += 1
        # request can id, max pwm of board
        bus.send("BMCConfigCmd", {"address": 0x00, "value": 0x00, "apply": 0x0}, node_id=0)
        bus.send("BMCConfigCmd", {"address": 0x24, "value": 0x00, "apply": 0x0}, node_id=0)
        # set max pwm of board
        bus.send("BMCConfigCmd", {"address": 0x24, "value": 100.0, "apply": 0x1}, node_id=0)
        # set motor en on board
        bus.send("BMCConfigCmd", {"address": 0x01, "value": 0b00000001, "apply": 0x1}, node_id=0)
        # request pwm, motor en of board
        bus.send("BMCConfigCmd", {"address": 0x24, "value": 0x00, "apply": 0x0}, node_id=0)
        bus.send("BMCConfigCmd", {"address": 0x01, "value": 0x00, "apply": 0x0}, node_id=0)
        # set mode to throttle
        bus.send("BMCModeCmd", {"mode": 5, "enable": 1}, node_id=0)
        sleep(2)
        for _ in range(5):
            bus.send("BMCTargetCmd", {"target": 10, "target_valid": 1}, node_id=0)
            sleep(5)
        for _ in range(5):
            bus.send("BMCTargetCmd", {"target": -10, "target_valid": 1}, node_id=0)
            sleep(5)
        sleep(5)
