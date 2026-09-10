# Board Name (ACRONYM)

<!--
Template for a board page. Copy this file, fill it in, then add it to the `nav` in zensical.toml.
This page is deliberately not in the nav.

Keep it short. A reader should be able to answer three questions quickly:
  1. What does this board physically do?
  2. What CAN messages does it speak?
  3. How do I build, flash and configure it?

Delete any section that does not apply rather than leaving it empty.
-->

One or two sentences: what the board is for and where it sits on the rover. Firmware lives in
`src/<name>`.

## Hardware

What the board is connected to (sensors, actuators, switches) and over which peripheral.

| Interface | Peripheral | Notes |
| --- | --- | --- |
| | | |

## Behavior

How the firmware actually operates: what runs on a timer, what runs in the main loop, what is
interrupt driven. Include the rates, since those are the numbers people come looking for.

| Timer | Rate | Purpose |
| --- | --- | --- |
| | | |

Describe any state machine, fault handling or calibration sequence here.

## CAN Interface

### Receives

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| | | | |

### Sends

| Message | Base ID | DLC | Signals |
| --- | --- | --- | --- |
| | | | |

All boards with a configuration also handle `ESWConfigCmd` and `ESWProbe`, and reply with
`ESWAck`. See [Boards](index.md#shared-messages).

## Configuration

If the board has a `config/<name>.yaml`, describe its registers here and link to the schema
reference. If it does not, say so explicitly, since readers will look.

Definition: `config/<name>.yaml`. Device values: `rover/<subsystem>/<device>.yaml`.

| Register | Type | Meaning |
| --- | --- | --- |
| | | |

See [Register Definitions](../../reference/config/schema.md).

## Building

```bash
./scripts/build.sh --src src/<name> --preset Debug
./scripts/build.sh --src src/<name> --preset Debug --flash
```

Note whether the board is in `ci.json`; if it is not, say that it is not built by CI.

## Known Gaps

Anything unfinished, unverified or surprising that would cost someone an afternoon.
