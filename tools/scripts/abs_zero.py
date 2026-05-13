from time import sleep

from esw import esw_logger
from esw.can.canbus import CANBus
from esw.can.dbc import get_dbc

if __name__ == "__main__":
    LOOP_DELAY = 0.05
    PITCH_CAN_ID = 0x37
    ROLL_CAN_ID = 0x38
    SRC_ID = 0x10

    def on_msg_recv(msg):
        msg_name, signals, src_id, dest_id = msg
        if int(src_id) == PITCH_CAN_ID:
            esw_logger.info(f"[PITCH] CAN RECV {msg_name}: {signals}")
        elif int(src_id) == ROLL_CAN_ID:
            esw_logger.info(f"[ ROLL] CAN RECV {msg_name}: {signals}")

    with CANBus(get_dbc(dbc_name="MRoverCAN"), "can1", on_recv=on_msg_recv) as bus:
        bus.send("ABSZeroCmd", {"offset": 0.0}, src_id=SRC_ID, dest_id=PITCH_CAN_ID)
        bus.send("ABSZeroCmd", {"offset": 0.0}, src_id=SRC_ID, dest_id=ROLL_CAN_ID)
        sleep(10)
