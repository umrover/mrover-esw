from dataclasses import dataclass


# TypeInfo dataclass, used for validating config size
@dataclass
class TypeInfo:
    type: str
    size: int


types: dict[str, TypeInfo] = {
    "int": TypeInfo("int", 4),
    "uint64": TypeInfo("uint64_t", 8),
    "uint32": TypeInfo("uint32_t", 4),
    "uint16": TypeInfo("uint16_t", 2),
    "uint8": TypeInfo("uint8_t", 1),
    "float32": TypeInfo("float", 4),
    "float64": TypeInfo("double", 8),
}


# ChipInfo dataclass, need page size, want config size to be < 1 page
@dataclass
class ChipInfo:
    flash_begin: int
    flash_end: int
    page_size: int
    num_pages: int


chips: dict[str, ChipInfo] = {"STM32G431RBTx": ChipInfo(0x08000000, 0x0801FFFF, 2048, 64)}


# shorthand for can id types
can_id_types: dict[str, str] = {"ext": "Extended"}


@dataclass
class RegGenResult:
    reg_output: str
    field_output: str
    reg_names: list[str]
    config_size: int
