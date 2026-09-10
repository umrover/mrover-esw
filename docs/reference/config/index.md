# Configuration Overview

Every custom ESW board keeps its settings in a small block of flash that can be rewritten over CAN
without reflashing firmware. That is how a board gets commissioned: the same firmware image goes on
every ABS board, and each physical board is then given its own CAN ID, encoder offsets and limits.

## Two Layers

Configuration is split into a definition and a set of values, and they live in different places.

| | Definition | Values |
| --- | --- | --- |
| Where | `config/<board>.yaml` | `rover/<subsystem>/<device>.yaml` |
| Describes | register names, types, bit positions | what one physical board should be set to |
| One per | board type (`abs`, `bmc`, `lim`) | installed device (`abs_de_pitch`, `joint_b`, ...) |
| Consumed at | build time, generating a C++ header | commissioning time, sent over CAN |

Each layer reaches the board by its own path. Taking the ABS encoder as the example:

- **The definition is compiled in.** `config/abs.yaml` is read at build time by `config_gen.py`,
  which generates `src/abs/Inc/abs_config.hpp`. That header becomes the register struct the
  firmware is built against, so the layout ships inside the image.
- **The values are pushed over CAN.** `rover/ra/abs_de_pitch.yaml` is read at commissioning time by
  `config.py`, which sends one `ESWConfigCmd` frame per register. The board writes each one into
  its last flash page, where the values persist across power cycles.

Both paths read the register layout from the same definition file, which is what keeps them
agreeing.

## Reading Further

- [Register Definitions](schema.md): the `config/*.yaml` schema and the generated C++ API
- [Device Values](devices.md): the `rover/` tree, and the separate moteus format
- [CAN Configuration Interface](can-interface.md): pushing values to a board
- [Boards](../../projects/boards/index.md): per-board registers and CAN messages

## A Note on Moteus

Brushless joints use mjbots moteus controllers, which have their own configuration system entirely.
Those are the `rover/mob/*.cfg` files, and none of the above applies to them. See
[Device Values](devices.md#moteus-controllers-cfg).
