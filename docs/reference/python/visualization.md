# `esw.visualization`

Live plotting for bench testing, so you can watch a target and an actual value converge while a
control loop runs.

!!! note
    Nothing currently uses this module. The only reference is commented out in
    `tools/scripts/send_can.py`. It is documented here because it works and is the obvious tool
    for tuning a control loop, but expect to be its first user.

## AsyncPlotter

A context manager that runs matplotlib in a separate process, so plotting cannot stall the loop
feeding it data.

```python
AsyncPlotter(
    labels: tuple[str, ...] = ("Target", "Actual"),
    max_size: int = 200,
    loop_delay: float = 0.05,
    x_label: str = "",
    y_label: str = "",
)
```

| Argument | Meaning |
| --- | --- |
| `labels` | one line per label, and the arity `send_data` expects |
| `max_size` | samples retained; older ones scroll off |
| `loop_delay` | redraw interval in seconds |
| `x_label`, `y_label` | axis labels |

| Method | Signature |
| --- | --- |
| `send_data` | `send_data(*values: float)` |

`send_data` timestamps each sample relative to when the plotter started. Passing the wrong number
of values logs a warning rather than raising.

## Usage

```python
from esw.can.canbus import CANBus
from esw.can.dbc import get_dbc
from esw.visualization.async_plotter import AsyncPlotter

TARGET = 1.0

with AsyncPlotter(labels=("Target", "Actual"), y_label="position (rad)") as plot:
    with CANBus(get_dbc(dbc_name="MRoverCAN"), "can0") as bus:
        while True:
            bus.send("BMCTargetCmd", {"target": TARGET, "target_valid": 1}, dest_id=0x67)
            msg = bus.recv(timeout=0.1)
            if msg and msg[0] == "BMCMotorState":
                plot.send_data(TARGET, msg[1]["position"])
```

The child process ignores `SIGINT`, so `Ctrl+C` is handled by the parent and the plot window shuts
down with it.
