# Register Definitions

A board's register layout lives in `config/<project>.yaml`, where `<project>` matches the firmware
directory name under `src/`.

## Schema

| Key | Required | Meaning |
| --- | --- | --- |
| `struct_name` | yes | name of the generated C++ struct, e.g. `abs_config_t` |
| `chip` | yes | part number, used for the flash geometry. |
| `regs` | yes | ordered list of registers. **Order determines addresses** |
| `can_filtering` | yes | FDCAN hardware filter and timing settings |

### `regs`

| Key | Required | Meaning |
| --- | --- | --- |
| `name` | yes | register name, uppercased for the C++ member |
| `type` | yes | see the type table below |
| `fields` | no | bit fields within the register |

Each entry under `fields` has a `name` and a `pos` (bit position, counting from the LSB). A
register with `fields` is addressed only through those fields; one without gets a whole-register
accessor instead.

### `can_filtering`

| Key | Required | Meaning |
| --- | --- | --- |
| `id_reg` | yes | register holding this node's CAN ID. Must name a declared register |
| `id_type` | yes | `ext` is the only supported value |
| `delay_compensation` | yes | transceiver delay compensation, needed for bit rate switching |
| `tdc_offset` | yes | transmitter delay compensation offset |
| `tdc_filter` | yes | transmitter delay compensation filter window |
| `can_subs` | no | additional subscribe filters, each with `can_id` and `id_type` |

## Types

| YAML type | C++ type | Bytes | Generates header | Can be flashed |
| --- | --- | --- | --- | --- |
| `uint8` | `uint8_t` | 1 | yes | yes |
| `uint16` | `uint16_t` | 2 | yes | yes |
| `uint32` | `uint32_t` | 4 | yes | yes |
| `float32` | `float` | 4 | yes | yes |

## Size Limit

The whole config must fit in one flash page. On the STM32G431CB that is 2048 bytes, and the
generator raises an error if the registers exceed it.

## Example

`config/abs.yaml`:

```yaml
struct_name: abs_config_t
chip: STM32G431CBTx
regs:
- name: can_id
  type: uint8
- name: host_can_id
  type: uint8
- name: sys_cfg
  type: uint16
  fields:
  - name: continuous_mode
    pos: 0
  - name: bounded_mode
    pos: 1
  - name: invert
    pos: 2
- name: output_scalar
  type: float32
can_filtering:
  id_reg: can_id
  id_type: ext
  delay_compensation: true
  tdc_offset: 13
  tdc_filter: 1
```

Addresses fall out of the order: `can_id` at `0x0`, `host_can_id` at `0x1`, `sys_cfg` at `0x2`
(two bytes), `output_scalar` at `0x4`.

## Generated Header

`config_gen.py` renders this into `src/abs/Inc/abs_config.hpp`, which defines:

```cpp
struct abs_config_t {
    reg_t<uint8_t>  CAN_ID{0x0};
    reg_t<uint8_t>  HOST_CAN_ID{0x1};
    reg_t<uint16_t> SYS_CFG{0x2};
    reg_t<float>    OUTPUT_SCALAR{0x4};

    using continuous_mode = field_t<&abs_config_t::SYS_CFG, 0>;
    using bounded_mode    = field_t<&abs_config_t::SYS_CFG, 1>;
    using invert          = field_t<&abs_config_t::SYS_CFG, 2>;
    using output_scalar   = field_t<&abs_config_t::OUTPUT_SCALAR>;

    template<typename F> auto get() const;
    template<typename F> void set(auto value);

    auto set_raw(uint8_t address, uint32_t raw) -> bool;
    auto get_raw(uint8_t address, uint32_t& raw) const -> bool;
};
```

Firmware reads and writes it by field:

```cpp
if (config.get<abs_config_t::invert>()) {
    position = -position;
}
```

`set_raw` and `get_raw` dispatch by address and are what the CAN handler calls. Values persist in
the **last flash page** of the chip: a write reads the page into RAM, patches the register, erases
the page and reprograms it.

A compile-time validator checks that no two registers overlap, so a malformed layout fails the
build rather than corrupting flash.

## Adding a Register

1. Append it to `regs` in `config/<board>.yaml`. **Append, do not insert**, since inserting shifts every
   later address and invalidates every already-configured board.
2. Rebuild the firmware so the header regenerates.
3. Add the key to each device file under `rover/` that needs a non-default value.
4. Reflash and reconfigure the affected boards.
