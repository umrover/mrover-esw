# Device Values

`rover/` holds one file per physical device on the rover, grouped by subsystem. These are the
values pushed to a board at commissioning time, against the layout defined in
`config/<board>.yaml`.

## Layout

| Directory | Subsystem | Contents |
| --- | --- | --- |
| `rover/ra/` | robotic arm | joint configs, absolute encoders, gripper, pusher |
| `rover/mob/` | mobility | six drive controllers plus a backup |
| `rover/sp/` | science payload | auger, linear actuator |
| `rover/cm/` | chassis and mounts | mast limit |
| `rover/test/` | bench testing | scratch configs |

Two different file formats live here, and they are not interchangeable.

## ESW Board Values (`.yaml`)

A `.yaml` file is a flat map of field name to value. The names come from the board's definition
file: for a register with `fields`, use the field names; for a plain register, use the register
name in lower case.

`rover/ra/abs_de_pitch.yaml`, an absolute encoder board:

```yaml
# can
can_id: 55
host_can_id: 16

# encoder settings
continuous_mode: false
bounded_mode: true
invert: true
output_scalar: 1.00
position_offset: 0.00
poll_frequency: 10.00
publish_frequency: 10.00
min_bound: -3.00
max_bound: 3.00
```

Every key here appears in `config/abs.yaml`, either as a register or as a field of `sys_cfg`.

!!! warning
    **Missing keys silently default to 0.** There is no check that a device file covers every
    register in its definition. A typo in a key name means that setting is quietly written as
    zero, not rejected. When adding a register, audit every device file that uses that board.

To push one of these to a board, see [CAN Configuration Interface](can-interface.md).

## Moteus Controllers (`.cfg`)

`rover/mob/*.cfg` are a completely separate system. Brushless drive joints use mjbots moteus
controllers, which have their own register namespace and their own tooling.

```
uuid.uuid.0 106
uuid.uuid.1 245
clock.hsitrim 64
aux1.i2c.i2c_hz 400000
aux1.i2c.devices.0.type 0
aux1.i2c.devices.0.address 64
```

These are flat `key value` lines using moteus register paths, not YAML, and nothing in `config/`
or `tools/esw/config` reads them. They are applied with mjbots' own tools (`moteus_tool`, or the
`moteus-gui` diagnostic console), both of which are dependencies of this repo's Python
environment.

See [Brushless Motors](../../info/brushless.md) for background on the controllers themselves.
