from curses.ascii import CAN
from time import sleep

from esw.can.dbc import get_dbc
from esw.can.canbus import CANBus


if __name__ == "__main__":
    with CANBus(get_dbc(dbc_name="CANBus1"), "vcan0") as bus:
        for i in range(100):
            bus.send("BMCProbe", {"data": 67 + i}, node_id=5)
            sleep(5)
