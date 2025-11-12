#!/usr/bin/env python3
"""
SWV_debug.py — Real-time plot of encoder angle & velocity via UART (CSV lines).
Firmware should print lines in the format:
    <t_ms>,<angle_deg>,<velocity_deg_per_s>\n
Example:
    0,12.341,1.210
    10,12.390,1.230
"""

import sys
import argparse
import collections
import time

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
    p.add_argument("--timeout", type=float, default=1.0, help="Serial read timeout (s)")
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

    # Rolling buffers
    N = args.points
    t_buf = collections.deque(maxlen=N)
    a_buf = collections.deque(maxlen=N)
    v_buf = collections.deque(maxlen=N)

    # Matplotlib setup
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(9, 6))
    fig.canvas.manager.set_window_title(f"UART Plot — {args.port} @ {args.baud} baud")

    line1, = ax1.plot([], [], lw=1)
    line2, = ax2.plot([], [], lw=1)

    ax1.set_ylabel("Angle (deg)")
    ax2.set_ylabel("Velocity (deg/s)")
    ax2.set_xlabel("Time (ms)")

    ax1.grid(True, linestyle="--", alpha=0.3)
    ax2.grid(True, linestyle="--", alpha=0.3)

    # To smooth occasional long pauses, we allow a few empty frames
    empty_reads_allowed = 5
    empty_reads = 0

    def update(_frame_idx):
        nonlocal empty_reads
        line = ser.readline().decode("utf-8", errors="ignore").strip()

        if not line:
            # no data this tick
            empty_reads += 1
            if empty_reads > empty_reads_allowed:
                # keep axes from collapsing
                if len(t_buf) > 1:
                    ax1.set_xlim(t_buf[0], t_buf[-1])
                    ax2.set_xlim(t_buf[0], t_buf[-1])
                empty_reads = 0
            return line1, line2

        try:
            # Expect CSV: t_ms,angle_deg,vel_dps
            # Allow spaces: split then strip
            parts = [p.strip() for p in line.split(",")]
            if len(parts) < 3:
                raise ValueError("not enough columns")
            t_ms = float(parts[0])
            ang  = float(parts[1])
            vel  = float(parts[2])
        except Exception:
            if args.print_bad:
                print(f"[WARN] Bad line: {repr(line)}")
            return line1, line2

        empty_reads = 0
        t_buf.append(t_ms)
        a_buf.append(ang)
        v_buf.append(vel)

        # Update data
        line1.set_data(t_buf, a_buf)
        line2.set_data(t_buf, v_buf)

        # Dynamic axes
        if len(t_buf) > 5:
            xmin, xmax = t_buf[0], t_buf[-1]
            ax1.set_xlim(xmin, xmax)
            ax2.set_xlim(xmin, xmax)

            # Let matplotlib recalc y-lims
            ax1.relim(); ax1.autoscale_view(scalex=False, scaley=True)
            ax2.relim(); ax2.autoscale_view(scalex=False, scaley=True)

        return line1, line2

    # Keep a reference to avoid GC; disable frame caching (avoids warning)
    anim = FuncAnimation(fig, update, interval=10, blit=True, cache_frame_data=False)

    try:
        plt.tight_layout()
        plt.show()
    except KeyboardInterrupt:
        pass
    finally:
        try:
            ser.close()
        except Exception:
            pass

if __name__ == "__main__":
    main()
