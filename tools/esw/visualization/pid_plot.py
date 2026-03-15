import matplotlib.pyplot as plt
from multiprocessing import Queue, Event
from time import sleep
import queue
import signal


def plotting_process(data_queue: Queue, stop_event: Event, max_size: int = 200, loop_delay: float = 0.05):
    # ignore ctrl+c - main process to stop first
    signal.signal(signal.SIGINT, signal.SIG_IGN)

    plt.ion()
    fig, ax = plt.subplots()
    times, targets, actuals = [], [], []

    line_target, = ax.plot([], [], 'r--', label='Target')
    line_actual, = ax.plot([], [], 'b-', label='Actual')
    ax.legend(loc='upper right')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Unit')

    while not stop_event.is_set():
        while True:
            try:
                timestamp, target_val, actual_val = data_queue.get_nowait()
                times.append(timestamp)
                targets.append(target_val)
                actuals.append(actual_val)
            except queue.Empty:
                break

        if times:
            if len(times) > max_size:
                times, targets, actuals = times[-max_size:], targets[-max_size:], actuals[-max_size:]

            line_target.set_data(times, targets)
            line_actual.set_data(times, actuals)
            ax.relim()
            ax.autoscale_view()

        fig.canvas.draw()
        fig.canvas.flush_events()
        sleep(loop_delay)

    fig.canvas.draw()
    plt.ioff()
    plt.show()