from time import sleep

from esw import esw_logger
from esw.can.canbus import CANBus, float2bits
from esw.can.dbc import get_dbc
from esw.visualization.async_plotter import AsyncPlotter

if __name__ == "__main__":
    LOOP_DELAY = 0.05
    CAN_ID = 0x67
    SRC_ID = 0x10

    config = {
        0x00: 0x38,  # can id
        0x01: 0x10,  # host can id
        0x02: 0x0000,  # user reg
        0x04: 1.00,  # output scalar
        0x08: 0.00,  # position offset
        0x0C: 10.0,  # poll frequency
        0x10: 10.0,  # publish frequency
    }
    # config = {
    #     0x00: 0x00,  # can id
    #     0x01: 0x00,  # host can id
    #     0x02: 0x00,  # noise margin
    #     0x03: 0x00,  # user reg
    #     0x04: 0x00,  # output scalar
    #     0x08: 0x00,  # position offset
    #     0x0C: 0x00,  # poll frequency
    #     0x10: 0x00,  # publish frequency
    # }

    # with AsyncPlotter(
    #     labels=(
    #         "Position (rad)",
    #         "Velocity (rad/s)",
    #     ),
    #     max_size=200,
    #     loop_delay=LOOP_DELAY,
    #     x_label="Time (s)",
    #     y_label="Waveform",
    # ) as plotter:

    def on_msg_recv(msg):
        msg_name, signals, src_id, dest_id = msg
        # plotter.send_data(float(signals["position"]), float(signals["velocity"]))
        esw_logger.info(f"CAN RECV {msg_name}: {signals}")

    with CANBus(get_dbc(dbc_name="MRoverCAN"), "can1", on_recv=on_msg_recv) as bus:
        bus.send("ESWProbe", {"data": 67}, src_id=SRC_ID, dest_id=CAN_ID)

        # write configs:
        for addr, value in config.items():
            val_bits: int
            if isinstance(value, float):
                val_bits = float2bits(value)
            else:
                val_bits = value
            bus.send("ESWConfigCmd", {"address": addr, "value": val_bits, "apply": 0x1}, dest_id=CAN_ID)
            sleep(LOOP_DELAY)

        sleep(2)
        sleep(9999999)
