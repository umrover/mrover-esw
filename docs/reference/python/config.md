# `esw.config`

Reads board register definitions and turns them into either a C++ header or a set of values to
push over CAN. For the file formats themselves see
[Register Definitions](../config/schema.md).

## `esw.config.types`

The shared tables. Two of them, and they do not agree. See the warning in
[Register Definitions](../config/schema.md#types).

| Symbol | Purpose |
| --- | --- |
| `TypeInfo(type: str, size: int)` | a C type and its width |
| `types: dict[str, TypeInfo]` | YAML type to C type, used by the **header generator** |
| `PyTypeInfo(size: int, pytype: type)` | a Python type and its width |
| `pytypes: dict[str, PyTypeInfo]` | YAML type to Python type, used by the **value packer** |
| `ChipInfo(flash_begin, flash_end, page_size, num_pages)` | flash geometry |
| `chips: dict[str, ChipInfo]` | one entry, `STM32G431CBTx` |
| `can_id_types: dict[str, str]` | `{"ext": "Extended"}` |

`RegGenResult` is also defined here but unused.

## `esw.config.parser`

Turns a definition plus a device file into register values.

```python
load_definition(definition_yaml)
parse_config(definition_yaml: Path, config_yaml: Path) -> dict[str, dict[str, int]]
display_config(registers: dict[str, dict[str, int]]) -> None
```

`load_definition` walks the `regs` list, assigning byte addresses sequentially from 0. Registers
with `fields` produce one entry per field; plain registers produce one entry covering the whole
width. An unknown type raises `ValueError`.

`parse_config` reads the device values and packs them into their registers: bool fields are OR-ed
in at their bit position, integers are masked to width and shifted, floats are kept as floats.
Returns `{REGISTER_NAME: {"addr": int, "value": int | float}}` ordered by address.

`display_config` prints that table for confirmation before anything is sent.

```python
from esw.config.parser import parse_config, display_config

cfg = parse_config(Path("config/abs.yaml"), Path("rover/ra/abs_de_pitch.yaml"))
display_config(cfg)
```

!!! warning
    A key present in the definition but absent from the device file is packed as **0**, with no
    warning. Nothing checks that a device file is complete.

## `esw.config.generator`

Renders the C++ header consumed by firmware.

```python
ConfigGen(project_name: str, tab_size: int, template_dir: Path)
    .generate_config_header(yaml_path: str | Path, output_path: str | Path) -> None
    .validate_can_filtering(can_filtering: dict, reg_names: list[str]) -> None
```

`generate_config_header` validates and renders in one pass. It checks that `struct_name`, `regs`,
`chip` and `can_filtering` are present, that `chip` is a known part, that every register type is
in `types`, and that the total size fits in one flash page. It then annotates each register with
its C type, byte size and address before rendering
`lib/config/templates/config_header.hpp.j2`.

`validate_can_filtering` requires `id_reg` (which must name a declared register), `id_type` (must
be a key of `can_id_types`, and is rewritten in place to the C++ enum name), `delay_compensation`,
`tdc_offset` and `tdc_filter`. An optional `can_subs` list each needs `can_id` and `id_type`.

The `tab_size` constructor argument is stored but never used.

Called from `tools/scripts/config_gen.py`, which CMake invokes during a firmware build. Running it
by hand:

```bash
uv run --project tools python tools/scripts/config_gen.py \
    --name abs --input config/abs.yaml \
    --output src/abs/Inc/abs_config.hpp \
    --template-dir lib/config/templates
```
