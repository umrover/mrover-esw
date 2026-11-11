# Boards

Every custom board on the rover runs firmware from a project under `src/`. This section documents
what each board does and the CAN messages it speaks.

| Board | Directory | Purpose | Config |
| --- | --- | --- | --- |
| [ABS](abs.md) | `src/abs` | absolute encoder | `config/abs.yaml` |
| [BMC](bmc.md) | `src/bmc` | brushed motor controller | `config/bmc.yaml` |
| [LIM](lim.md) | `src/lim` | standalone limit switch reporter | `config/lim.yaml` |
| [PDB](pdlb.md) | `src/pdlb` | power distribution and autonomy status LED | none |
| [SCI](science.md) | `src/science` | atmospheric sensor suite | none |

`src/tests/*` are firmware test fixtures rather than boards, and
`src/canalyzer` is an early-stage project developed on its own branch.

## CAN Addressing

Every message carries a base ID from the DBC plus a source and destination node ID packed into the
arbitration ID:

```
arbitration_id = base_id + (src_id << 8) + dest_id
```

The node IDs are the boards' configured `can_id` and `host_can_id`. The rover-side host is
`0x10`. See [esw.can](../../reference/python/can.md) for the host-side API.

Base IDs are grouped by board:

| Range | Board |
| --- | --- |
| `0x8010` - `0x8013` | BMC |
| `0x8020` - `0x8022` | ABS |
| `0x8030` - `0x8031` | LIM |
| `0x8051`, `0x8060` - `0x8061` | SCI |
| `0x8070` - `0x8071` | PDB |
| `0x80F0` - `0x80F2` | shared, all boards |

## Shared Messages

Three messages are common to the boards that have a configuration.

| Message | Base ID | DLC | Direction | Signals |
| --- | --- | --- | --- | --- |
| `ESWConfigCmd` | `0x80F00000` | 6 | to board | `address` (8), `value` (32), `apply` (1) |
| `ESWProbe` | `0x80F10000` | 4 | to board | `data` (32) |
| `ESWAck` | `0x80F20000` | 4 | from board | `data` (32) |

`ESWConfigCmd` writes one configuration register; with `apply` clear it reads the register back
instead, answering with `ESWAck`. `ESWProbe` is a liveness check that echoes its payload back in an
`ESWAck`.

PDB and SCI implement neither, since neither has a configuration file.

See [Configuration](../../reference/config/index.md) for how registers are defined and pushed.

## Adding a Board

Copy `_template.md` in this directory as the starting point for a new board page, then add it to
the `nav` in `zensical.toml`.
