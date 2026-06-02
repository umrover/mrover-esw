from time import sleep

from esw import esw_logger
from esw.can.canbus import CANBus
from esw.can.dbc import get_dbc

if __name__ == "__main__":
    LOOP_DELAY = 0.05
    CAN_ID = 49
    SRC_ID = 0x10

    def on_msg_recv(msg):
        msg_name, signals, src_id, dest_id = msg
        esw_logger.info(f"CAN RECV {msg_name}: {signals}")

    with CANBus(get_dbc(dbc_name="MRoverCAN"), "can1", on_recv=on_msg_recv) as bus:
        bus.send("BMCResetCmd", {"reset": 1, "clear_faults": 1}, src_id=SRC_ID, dest_id=CAN_ID)
        sleep(LOOP_DELAY)
