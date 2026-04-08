import argparse
from dataclasses import dataclass
import textwrap
from pathlib import Path
import yaml


# TypeInfo dataclass, used for validating config size
@dataclass
class TypeInfo:
    type: str
    size: int


types = {
    "int": TypeInfo("int", 4),
    "uint32_t": TypeInfo("uint32_t", 4),
    "uint16_t": TypeInfo("uint16_t", 2),
    "uint8_t": TypeInfo("uint8_t", 1),
    "float": TypeInfo("float", 4),
}


# ChipInfo dataclass, need page size, want config size to be < 1 page
@dataclass
class ChipInfo:
    flash_begin: int
    flash_end: int
    page_size: int
    num_pages: int


chips = {"g431cbt6": ChipInfo(0x08000000, 0x0801FFFF, 2048, 64)}


# shorthand for can id types
can_id_types = {"ext": "Extended"}


@dataclass
class RegGenResult:
    reg_output: str
    field_output: str
    reg_names: list[str]
    config_size: int


class ConfigGen:
    struct_name: str
    tab_size: int

    # get_raw function
    GET_RAW_FN: str = """
        auto get_raw(uint8_t address, uint32_t& raw) const -> bool {
            bool found = false;
            std::apply([&](auto const&... reg) -> void {
                ((reg.addr == address ? (raw = to_raw(reg.value.value_or(0)), found = true) : false), ...);
            },
                        all());
            return found;
        }\n"""

    # set_raw function
    SET_RAW_FN: str = """\
        auto set_raw(uint8_t address, uint32_t const raw) -> bool {
            bool found = false;

            std::apply(
                    [&](auto const&... reg) -> void {
                        (
                                [&] -> void {
                                    if (reg.addr == address) {
                                        using T = std::remove_reference_t<decltype(reg)>::value_t;
                                        reg.write(*this, from_raw<T>(raw));
                                        found = true;
                                    }
                                }(),
                                ...);
                    },
                    all());

            return found;
        }\n"""

    # size_bytes function
    SIZE_BYTES_FN: str = """\
        static consteval auto size_bytes() -> uint16_t {
            return validated_config_t<bmc_config_t>::size_bytes();
        }\n"""

    GET_FN: str = """\
        template<typename F>
        auto get() const { return F::get(*this); }\n"""

    SET_FN: str = """\
        template<typename F>
        void set(auto value) { F::set(*this, value); }\n"""

    def __init__(self, tab_size: int):
        self.tab_size = tab_size

    def generate_config_struct(self, yaml_path: str, tab_size: int) -> str:
        with open(yaml_path) as file:
            config = yaml.safe_load(file)
        self.struct_name = config["name"]
        regs: list[dict] = config["regs"]
        mem = chips[config["chip"]]

        tab: str = " " * tab_size

        output_lines: list[str] = []

        # imports and pragma
        output_lines.append("#pragma once\n")

        output_lines.append("#include <serial/fdcan.hpp>")
        output_lines.append("#include <serial/uart.hpp>")
        output_lines.append("#include <tuple>\n")

        output_lines.append("#include <CANBus1.hpp>")
        output_lines.append("#include <adc.hpp>")
        output_lines.append("#include <hw/ad8418a.hpp>")
        output_lines.append("#include <hw/flash.hpp>\n")

        output_lines.append("namespace mrover {\n")

        output_lines.append(tab + f"struct {self.struct_name} {{\n")

        output_lines.append(tab * 2 + "static inline void* flash_ptr = nullptr;\n")

        num_subs: int = 0
        if config["can_filtering"] is not None:
            num_subs = len(config["can_filtering"]["can_subs"])
            output_lines.append(tab * 2 + f"FDCAN::Filter can_node_filters[{num_subs + 1}];\n")

        result: RegGenResult = self.generate_regs_fields(regs, mem)
        reg_names: list[str] = result.reg_names

        output_lines += result.reg_output + result.field_output
        output_lines.append("")

        # add get/set, get/set_raw, size_bytes to output
        output_lines.append(self.GET_FN)
        output_lines.append(self.SET_FN)
        output_lines.append(self.generate_all_fn(reg_names))
        output_lines.append(self.SET_RAW_FN)
        output_lines.append(self.GET_RAW_FN)

        # memory layout struct
        output_lines.append(tab * 2 + "struct mem_layout {")
        output_lines.append(tab * 3 + f"static constexpr uint32_t FLASH_BEGIN_ADDR = {mem.flash_begin:#x};")
        output_lines.append(tab * 3 + f"static constexpr uint32_t FLASH_END_ADDR = {mem.flash_end:#x};")
        output_lines.append(tab * 3 + f"static constexpr int PAGE_SIZE = {mem.page_size};")
        output_lines.append(tab * 3 + f"static constexpr int NUM_PAGES = {mem.num_pages};")
        output_lines.append(tab * 2 + "};\n")

        output_lines.append(self.SIZE_BYTES_FN)

        # end of config struct
        output_lines.append(tab + f"}}; // {self.struct_name}\n")

        output_lines.append(self.generate_can_fn(config["can_filtering"]))

        output_lines.append("} // namespace mrover")

        return "\n".join(output_lines)

    def generate_regs_fields(self, regs: list[dict], chip: ChipInfo) -> RegGenResult:
        output_regs: list[str] = []
        output_fields: list[str] = []
        reg_names: list[str] = []

        tab: str = self.tab_size * " "

        current_pos: int = 0
        last_had_fields: bool = False
        for reg in regs:
            name: str = reg["name"].upper()
            reg_names.append(name)
            typ: TypeInfo | None = types.get(reg["type"], None)
            if typ is None:
                raise ValueError(f"Unsupported reg type: {reg['type']}")
            fields: list[dict] | None = reg.get("fields")

            if fields is not None:
                output_fields.append("")
                for field in fields:
                    field_name: str = field["name"]
                    field_size: int | None = reg.get("size")

                    if field_size is None:
                        field_size = 1  # default to field size of 1 if not provided in yaml

                    field_pos = field["pos"]
                    line = tab * 2 + f"using {field_name} = field_t<&{self.struct_name}::{name}, {field_pos}"
                    if field_size != 1:
                        line += f", {field_size}"
                    line += ">;"

                    output_fields.append(line)
                last_had_fields = True
            else:
                # no fields provided
                if last_had_fields:
                    output_fields.append("")  # this block is just to make things look nice
                    last_had_fields = False
                if typ.type == "float":
                    output_fields.append(tab * 2 + f"using {name.lower()} = field_t<&{self.struct_name}::{name}>;")
                else:
                    output_fields.append(
                        tab * 2 + f"using {name.lower()} = field_t<&{self.struct_name}::{name}, 0, {typ.size * 8}>;"
                    )

            output_regs.append(tab * 2 + f"reg_t<{typ.type}> {name}{{{current_pos:#x}}};")

            current_pos += typ.size
            # check that config will fit into last page
            if current_pos >= chip.page_size:
                raise ValueError(f"Config does not fit in one page: size {current_pos} exceeds {chip.page_size}")

        output_regs.append("")
        return RegGenResult("\n".join(output_regs), "\n".join(output_fields), reg_names, current_pos)

    def generate_all_fn(self, regs: list[str]) -> str:
        output_lines: list[str] = []
        tab: str = self.tab_size * " "
        # non-const all function
        output_lines.append(tab * 2 + "constexpr auto all() {")
        output_lines.append(tab * 3 + "return std::forward_as_tuple(")
        # use textwrap to join reg names to fit on multiple lines
        joined_names = ", ".join(regs)
        wrapped = textwrap.fill(
            joined_names, width=80, initial_indent=" " * self.tab_size * 4, subsequent_indent=" " * self.tab_size * 4
        )
        output_lines.append(wrapped)
        output_lines.append(tab * 3 + ");")
        output_lines.append(tab * 2 + "}\n")

        # const all function
        output_lines.append(tab * 2 + "constexpr auto all() const {")
        output_lines.append(tab * 3 + "return std::forward_as_tuple(")
        output_lines.append(wrapped)
        output_lines.append(tab * 3 + ");")
        output_lines.append(tab * 2 + "}\n")

        return "\n".join(output_lines)

    def generate_can_fn(self, can_filtering: dict) -> str:
        output_lines: list[str] = []
        tab: str = self.tab_size * " "
        output_lines.append(tab + f"inline auto get_can_options({self.struct_name}* config) -> FDCAN::Options {{")

        if can_filtering is None:
            output_lines.append(tab * 2 + "auto can_opts = FDCAN::Options{};")
            output_lines.append(tab * 2 + "can_opts.delay_compensation = true;")
            output_lines.append(tab * 2 + "can_opts.tdc_offset = 13;")
            output_lines.append(tab * 2 + "can_opts.tdc_filter = 1;")
            output_lines.append(tab * 2 + "return can_opts;\n")
            output_lines.append(tab + "}\n")
            return "\n".join(output_lines)

        can_reg = can_filtering["id_reg"]
        output_lines.append(
            tab * 2 + f"config->can_node_filters[0].id1 =  config->get<{self.struct_name}::{can_reg.lower()}()>;"
        )
        output_lines.append(tab * 2 + "config->can_node_filters[0].id2 = CAN_DEST_ID_MASK;")
        dest_id_type: str | None = can_id_types.get(can_filtering["id_type"], None)
        if dest_id_type is None:
            raise ValueError(f"Unsuported CAN id type: {can_filtering['id_type']}")
        output_lines.append(tab * 2 + f"config->can_node_filters[0].id_type = FDCAN::FilterIdType::{dest_id_type};")
        output_lines.append(tab * 2 + "config->can_node_filters[0].action = FDCAN::ActionType::Accept;")
        output_lines.append(tab * 2 + "config->can_node_filters[0].mode = FDCAN::ActionType::Mask;\n")

        num_subs = 0
        if can_filtering["can_subs"] is not None:
            num_subs = len(can_filtering["can_subs"])
            sub_num = 1
            for filter in can_filtering["can_subs"]:
                output_lines.append(tab * 2 + f"config->can_node_filters[{sub_num}].id1 = {filter['id']};")
                output_lines.append(tab * 2 + f"config->can_node_filters[{sub_num}].id2 = CAN_SRC_ID_MASK;")
                src_id_type: str | None = can_id_types.get(filter["type"], None)
                if src_id_type is None:
                    raise ValueError(f"Unsuported CAN id type: {filter['type']}")
                output_lines.append(
                    tab * 2 + f"config->can_node_filters[{sub_num}].id_type = FDCAN::FilterIdType::{src_id_type};"
                )
                output_lines.append(
                    tab * 2 + f"config->can_node_filters[{sub_num}].action = FDCAN::ActionType::Accept;"
                )
                output_lines.append(tab * 2 + f"config->can_node_filters[{sub_num}].mode = FDCAN::ActionType::Mask;\n")
                sub_num += 1

        output_lines.append(tab * 2 + "FDCAN::FilterConfig filter;")
        output_lines.append(tab * 2 + "filter.begin = config->can_node_filters;")
        output_lines.append(tab * 2 + f"filter.end = config->can_node_filters + {num_subs + 1};")
        output_lines.append(tab * 2 + "filter.global_non_matching_std_action = FDCAN::FilterAction::Reject;")
        output_lines.append(tab * 2 + "filter.global_non_matching_ext_action = FDCAN::FilterAction::Reject;\n")
        output_lines.append(tab * 2 + "auto can_opts = FDCAN::Options{};")
        output_lines.append(tab * 2 + "can_opts.delay_compensation = true;")
        output_lines.append(tab * 2 + "can_opts.tdc_offset = 13;")
        output_lines.append(tab * 2 + "can_opts.tdc_filter = 1;")
        output_lines.append(tab * 2 + "can_opts.filter_config = filter;")
        output_lines.append(tab * 2 + "return can_opts;\n")
        output_lines.append(tab + "}\n")

        return "\n".join(output_lines)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Generate Config Struct from yaml")
    parser.add_argument(
        "--input",
        "-i",
        type=Path,
        required=True,
        help="Path to yaml file describing config",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=argparse.FileType("w"),
        required=True,
        help="Path to file where config will be generated",
    )
    parser.add_argument(
        "--tabsize",
        type=int,
        required=False,
        default=4,
        help="Tab size in output file",
    )

    args = parser.parse_args()

    gen: ConfigGen = ConfigGen(args.tabsize)

    print(gen.generate_config_struct(args.input, args.tabsize), file=args.output)
