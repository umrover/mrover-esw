from multiprocessing import Process, Queue, Event
from time import sleep, time

from esw import esw_logger
from esw.can.canbus import CANBus
from esw.can.dbc import get_dbc
from esw.visualization.pid_plot import plotting_process

NUM_LOOPS = 50
LOOP_DELAY = 0.05
TARGET = -0.012
CAN_ID = 49
SRC_ID = 16


def on_msg_recv(msg, data_queue, start_time):
    msg_name, signals, src_id, dest_id = msg
    
    if msg_name == "BMCMotorState":
        current_time = time() - start_time
        actual_vel = signals.get('velocity', 0)
        # send data to plotting process
        data_queue.put((current_time, TARGET, actual_vel))
    
    esw_logger.info(f"CAN RECV {msg_name}: {signals}")

if __name__ == "__main__":
    # data structures to send data to process
    data_queue = Queue()
    stop_event = Event()
    start_time = time()

    # async plotting process
    p = Process(target=plotting_process, args=(data_queue, stop_event))
    p.start()

    try:
        # on_recv callback with plotting
        callback = lambda m: on_msg_recv(m, data_queue, start_time)
        
        with CANBus(get_dbc(dbc_name="MRoverCAN"), "can2", on_recv=callback) as bus:
            print("VELOCITY")
            
            # set mode to velocity
            for _ in range(5):
                bus.send("BMCModeCmd", {"mode": 7, "enable": 1}, src_id=SRC_ID, dest_id=CAN_ID)
            
            while True:
                for _ in range(NUM_LOOPS):
                    bus.send("BMCTargetCmd", {"target": TARGET, "target_valid": 1}, src_id=SRC_ID, dest_id=CAN_ID)
                    sleep(LOOP_DELAY)

    except KeyboardInterrupt:
        print("stop triggered")
    finally:
        stop_event.set()  # stop plotter
        p.join()  # stall on plotter