import queue
import signal
from multiprocessing import Queue, Event, Process
from multiprocessing.synchronize import Event as EventType
from time import sleep, time

import matplotlib.pyplot as plt

from esw import esw_logger


class AsyncPlotter:
    _data_queue: Queue
    _stop_event: EventType
    _start_time: float

    _process: Process | None

    _max_size: int
    _loop_delay: float
    _x_label: str
    _y_label: str
    _labels: tuple[str, ...]

    def __init__(
        self,
        labels: tuple[str, ...] = ("Target", "Actual"),
        max_size: int = 200,
        loop_delay: float = 0.05,
        x_label: str = "",
        y_label: str = "",
    ):
        self._data_queue = Queue()
        self._stop_event = Event()

        self._process = None

        self._labels = labels
        self._max_size = max_size
        self._loop_delay = loop_delay
        self._x_label = x_label
        self._y_label = y_label

    def __enter__(self):
        def plotting_process(
            data_queue: Queue,
            stop_event: EventType,
            labels: tuple[str, ...],
            max_size: int = 200,
            loop_delay: float = 0.05,
            x_label: str = "",
            y_label: str = "",
        ):
            # ignore ctrl+c - main process to stop first
            signal.signal(signal.SIGINT, signal.SIG_IGN)

            plt.ion()
            fig, ax = plt.subplots()

            times = []
            # Create a separate list for each series based on the number of labels
            series_data = [[] for _ in labels]

            # Dynamically create lines for each label. Matplotlib will auto-cycle colors.
            lines = [ax.plot([], [], label=lbl)[0] for lbl in labels]

            ax.legend(loc="upper right")
            ax.set_xlabel(x_label)
            ax.set_ylabel(y_label)

            while not stop_event.is_set():
                while True:
                    try:
                        data = data_queue.get_nowait()
                        times.append(data[0])  # Timestamp is always index 0

                        # Append remaining values to their respective series
                        for i, val in enumerate(data[1:]):
                            series_data[i].append(val)
                    except queue.Empty:
                        break

                if times:
                    if len(times) > max_size:
                        times = times[-max_size:]
                        series_data = [s[-max_size:] for s in series_data]

                    for line, s in zip(lines, series_data):
                        line.set_data(times, s)

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
            self._process = Process(
                target=plotting_process,
                args=(
                    self._data_queue,
                    self._stop_event,
                    self._labels,
                    self._max_size,
                    self._loop_delay,
                    self._x_label,
                    self._y_label,
                ),
            )
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

    def send_data(self, *values: float):
        if len(values) != len(self._labels):
            esw_logger.warning(f"AsyncPlotter expected {len(self._labels)} values, but got {len(values)}.")

        self._data_queue.put((time() - self._start_time, *values))
