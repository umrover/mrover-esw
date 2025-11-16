#!/usr/bin/env python3
"""
SWV_debug.py — Real-time plot of encoder angle & velocity via UART (CSV lines).
Firmware should print lines in the format:
    <t_ms>,<angle_deg>,<velocity_deg_per_s>\n
"""

import sys
import argparse
import collections
import threading

try:
    import serial
except Exception as e:
    print("Missing dependency: pyserial. Install with: pip install pyserial")
    raise

import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation


# --------------------------- CLI ---------------------------

def parse_args():
    p = argparse.ArgumentParser(description="Real-time UART plotter for CSV stream: t_ms,angle_deg,vel_dps")
    p.add_argument("--port", default="/dev/ttyACM0",
                   help="Serial port (e.g. /dev/ttyACM0, /dev/ttyUSB0, COM3)")
    p.add_argument("--baud", type=int, default=115200, help="Baud rate")
    p.add_argument("--timeout", type=float, default=0.01,
                   help="Serial read timeout (s) — smaller helps reduce lag")
    p.add_argument("--points", type=int, default=1000, help="Points to keep in the rolling window")
    p.add_argument("--print-bad", action="store_true", help="Print lines that fail to parse")
    return p.parse_args()


# ---------------------- Serial helpers ---------------------

def open_serial(port: str, baud: int, timeout: float) -> serial.Serial:
    try:
        ser = serial.Serial(port=port, baudrate=baud, timeout=timeout)
        # Discard a partial line if present
        ser.readline()
        return ser
    except Exception as e:
        print(f"[ERROR] Failed to open serial port '{port}' @ {baud} baud: {e}")
        print("Tips:")
        print("  • Check the correct port name (ls /dev/ttyACM* /dev/ttyUSB* on Linux; Device Manager on Windows).")
        print("  • On Linux, add yourself to 'dialout' group and re-login:  sudo usermod -a -G dialout $USER")
        sys.exit(1)


# ------------------------ Main app -------------------------

def main():
    args = parse_args()

    ser = open_serial(args.port, args.baud, args.timeout)

    # Rolling buffers (for plotting)
    N = args.points
    t_buf = collections.deque(maxlen=N)   # relative time in ms (starts from 0)
    a_buf = collections.deque(maxlen=N)   # angle in degrees
    v_buf = collections.deque(maxlen=N)   # velocity in deg/s

    # Raw line queue + lock, used to pass data between threads
    raw_lines = collections.deque(maxlen=10_000)
    raw_lock = threading.Lock()
    running = True

    # Time offset so that plots always start at t=0 for each run
    t0 = None

    # ------------------ Background reader thread ------------------
    def reader_thread():
        nonlocal running
        while running:
            try:
                line = ser.readline()
            except Exception:
                # If reading from the serial port fails, exit the thread
                break

            if not line:
                continue

            line = line.decode("utf-8", errors="ignore").strip()
            if not line:
                continue

            with raw_lock:
                raw_lines.append(line)

    th = threading.Thread(target=reader_thread, daemon=True)
    th.start()

    # --------------------------- Plot ---------------------------

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6))
    fig.canvas.manager.set_window_title(f"UART Plot — {args.port} @ {args.baud} baud")

    line1, = ax1.plot([], [], lw=1)
    line2, = ax2.plot([], [], lw=1)

    ax1.set_ylabel("Angle (deg)")
    ax2.set_ylabel("Velocity (deg/s)")
    ax2.set_xlabel("Time (ms)")

    ax1.grid(True, linestyle="--", alpha=0.3)
    ax2.grid(True, linestyle="--", alpha=0.3)

    # In update() we process a small batch of data per frame
    MAX_LINES_PER_FRAME = 200  # Maximum number of lines to consume per frame

    def update(_frame_idx):
        nonlocal t0
        consumed_any = False

        # Consume up to MAX_LINES_PER_FRAME lines from the queue
        for _ in range(MAX_LINES_PER_FRAME):
            with raw_lock:
                if not raw_lines:
                    break
                line = raw_lines.popleft()

            if not line:
                continue

            try:
                parts = [p.strip() for p in line.split(",")]
                if len(parts) < 3:
                    raise ValueError("not enough columns")
                t_ms = float(parts[0])
                ang = float(parts[1])
                vel = float(parts[2])
            except Exception:
                if args.print_bad:
                    print(f"[WARN] Bad line: {repr(line)}")
                continue

            # Initialize time offset on first valid sample
            if t0 is None:
                t0 = t_ms

            # Store relative time so that each run starts from 0
            t_rel = t_ms - t0

            t_buf.append(t_rel)
            a_buf.append(ang)
            v_buf.append(vel)
            consumed_any = True

        # If this frame has no new data, just return the existing line objects
        if not consumed_any:
            return line1, line2

        # Update line data
        line1.set_data(t_buf, a_buf)
        line2.set_data(t_buf, v_buf)

        # Dynamic axes
        if len(t_buf) > 5:
            xmin, xmax = t_buf[0], t_buf[-1]
            ax1.set_xlim(xmin, xmax)
            ax2.set_xlim(xmin, xmax)

            ax1.relim()
            ax1.autoscale_view(scalex=False, scaley=True)
            ax2.relim()
            ax2.autoscale_view(scalex=False, scaley=True)

        return line1, line2

    # interval can be ~20–30 ms, which is already visually smooth
    anim = FuncAnimation(fig, update, interval=20, blit=True, cache_frame_data=False)

    try:
        plt.tight_layout()
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        # On exit, stop the reader thread and close the serial port
        running = False
        try:
            ser.close()
        except Exception:
            pass


if __name__ == "__main__":
    main()
