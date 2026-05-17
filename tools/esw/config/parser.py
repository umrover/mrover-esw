from pathlib import Path

import yaml

from esw.config.types import pytypes


def load_definition(definition_yaml):
    with open(definition_yaml, "r") as f:
        definition = yaml.safe_load(f)

    config_map = {}
    addr = 0
    for reg in definition["regs"]:
        reg_name = reg["name"]
        reg_type = reg["type"]
        if reg_type not in pytypes:
            raise ValueError(f"Unsupported type: {reg_type}")

        size = pytypes[reg_type].size
        pytype = pytypes[reg_type].pytype
        cpp_name = reg_name.upper()

        if "fields" in reg:
            for field in reg["fields"]:
                config_map[field["name"]] = {
                    "register": cpp_name,
                    "addr": addr,
                    "dtype": bool,
                    "offset": field["pos"],
                    "width": 1,
                }

        else:
            config_map[reg_name] = {
                "register": cpp_name,
                "addr": addr,
                "dtype": pytype,
                "offset": 0,
                "width": size * 8,
            }

        addr += size

    return config_map


def parse_config(definition_yaml: Path, config_yaml: Path) -> dict[str, dict[str, int]]:
    config_map = load_definition(definition_yaml)
    with open(config_yaml, "r") as f:
        user_data = yaml.safe_load(f)

    reg_states = {}
    for field, info in config_map.items():
        val = user_data.get(field, 0)
        addr = info["addr"]
        dtype = info["dtype"]
        if addr not in reg_states:
            reg_states[addr] = {
                "name": info["register"],
                "value": 0,
                "dtype": dtype,
            }

        if dtype is float:
            reg_states[addr]["value"] = float(val)
        elif dtype is bool:
            if bool(val):
                reg_states[addr]["value"] |= 1 << info["offset"]
        else:
            mask = (1 << info["width"]) - 1
            reg_states[addr]["value"] |= (int(val) & mask) << info["offset"]

    registers = {}
    for addr in sorted(reg_states.keys()):
        info = reg_states[addr]

        registers[info["name"]] = {
            "addr": addr,
            "value": info["value"],
        }

    return registers


def display_config(registers: dict[str, dict[str, int]]) -> None:
    print(f"{'Register':<24} {'Addr':<8} {'Value':<16}")
    print("-" * 52)

    for name, info in registers.items():
        value = info["value"]

        if isinstance(value, float):
            value_str = f"{value:.6f}"
        else:
            value_str = hex(value)

        print(f"{name:<24} 0x{info['addr']:02X}     {value_str:<16}")
