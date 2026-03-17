import queue
import signal
from multiprocessing import Queue, Event, Process
from time import sleep, time

import matplotlib.pyplot as plt

from esw import esw_logger


class AsyncPlotter:
    _data_queue: Queue
    _stop_event: Event
    _start_time: float

    _process: Process | None

    _max_size: int
    _loop_delay: float
    _x_label: str
    _y_label: str

    def __init__(self, max_size: int = 200, loop_delay: float = 0.05, x_label: str = "", y_label: str = ""):
        self._data_queue = Queue()
        self._stop_event = Event()

        self._process = None

        self._max_size = max_size
        self._loop_delay = loop_delay
        self._x_label = x_label
        self._y_label = y_label

    def __enter__(self):
        def plotting_process(data_queue: Queue, stop_event: Event, max_size: int = 200, loop_delay: float = 0.05, x_label: str = "", y_label: str = ""):
            # ignore ctrl+c - main process to stop first
            signal.signal(signal.SIGINT, signal.SIG_IGN)

            plt.ion()
            fig, ax = plt.subplots()
            times, targets, actual_vals = [], [], []

            line_target, = ax.plot([], [], 'r--', label='Target')
            line_actual, = ax.plot([], [], 'b-', label='Actual')
            ax.legend(loc='upper right')
            ax.set_xlabel(x_label)
            ax.set_ylabel(y_label)

            while not stop_event.is_set():
                while True:
                    try:
                        timestamp, target_val, actual_val = data_queue.get_nowait()
                        times.append(timestamp)
                        targets.append(target_val)
                        actual_vals.append(actual_val)
                    except queue.Empty:
                        break

                if times:
                    if len(times) > max_size:
                        times, targets, actual_vals = times[-max_size:], targets[-max_size:], actual_vals[-max_size:]

                    line_target.set_data(times, targets)
                    line_actual.set_data(times, actual_vals)
                    ax.relim()
                    ax.autoscale_view()

                fig.canvas.draw()
                fig.canvas.flush_events()
                sleep(loop_delay)

            fig.canvas.draw()
            plt.ioff()
            plt.show()

        if self._process is None:
            esw_logger.info("Starting Plotting Process")
            self._process = Process(target=plotting_process, args=(self._data_queue, self._stop_event, self._max_size, self._loop_delay, self._x_label, self._y_label))
            self._start_time = time()
            self._process.start()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if self._process is not None:
            self._stop_event.set()  # stop plotter
            self._process.join()  # stall on plotter
            self._process = None
            esw_logger.info("Stopped Plotting Process")

        if exc_type:
            esw_logger.error(f"{exc_type.__class__.__name__}: {exc_value}")
            return False
        return False

    def send_data(self, target_val: float, actual_val: float):
        self._data_queue.put((time() - self._start_time, target_val, actual_val))
