from time import sleep

from esw import esw_logger
from esw.can.canbus import CANBus
from esw.can.dbc import get_dbc
from esw.visualization.async_plotter import AsyncPlotter

if __name__ == "__main__":

    NUM_LOOPS = 50
    LOOP_DELAY = 0.05
    TARGET = -1.0
    CAN_ID = 54
    SRC_ID = 16

    with AsyncPlotter(max_size=200, loop_delay=LOOP_DELAY, x_label="Time (s)", y_label="Waveform") as plotter:
        def on_msg_recv(msg):
            msg_name, signals, src_id, dest_id = msg
            if msg_name == "BMCMotorState":
                actual_vel = signals.get('velocity', 0)
                plotter.send_data(TARGET, actual_vel)
            esw_logger.info(f"CAN RECV {msg_name}: {signals}")

        with CANBus(get_dbc(dbc_name="MRoverCAN"), "can2", on_recv=on_msg_recv) as bus:

            # set mode to velocity
            # for _ in range(5):
            #     bus.send("BMCModeCmd", {"mode": 7, "enable": 1}, src_id=SRC_ID, dest_id=CAN_ID)

            # set mode to throttle
            for _ in range(5):
                bus.send("BMCModeCmd", {"mode": 5, "enable": 1}, src_id=SRC_ID, dest_id=CAN_ID)

            while True:
                for _ in range(NUM_LOOPS):
                    bus.send("BMCTargetCmd", {"target": TARGET, "target_valid": 1}, src_id=SRC_ID, dest_id=CAN_ID)
                    sleep(LOOP_DELAY)
