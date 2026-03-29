import argparse
import textwrap
from pathlib import Path    
import yaml

# type name, size (bytes)
types = {
    "int" : ("int", 4),
    "uint32_t" : ("uint32_t", 4),
    "uint16_t": ("uint16_t", 2),
    "uint8_t" : ("uint8_t", 1),
    "float": ("float", 4),
}

# flash begin, flash end, flash size (bytes), number flash pages
chips = {
    "g431cbt6" : (0x08000000, 0x0801FFFF, 2048, 64)
}

can_id_types = {
    "ext": "Extended"
}

def generate_config_struct(yaml_path: str, tab_size: int) -> str:
    with open(yaml_path) as file:
        config = yaml.safe_load(file)
    struct_name = config["name"]
    names = []
    regs = config["regs"]
    mem = chips[config["chip"]]

    output_lines = [f"struct {struct_name} {{\n"]
    output_lines.append("static inline void* flash_ptr = nullptr;\n")

    num_subs = 0
    if (config["can_filtering"] is not None):
        num_subs = len(config["can_filtering"]["can_subs"]) 
        output_lines.append(tab_size * " " + f"FDCAN::Filter can_node_filters[{num_subs + 1}];\n")

    output_regs = []
    output_fields = []

    current_pos = 0

    last_had_fields = False
    for reg in regs:

        name = reg["name"].upper()
        names.append(name)
        typ = types.get(reg["type"], 0)
        if (typ == 0):
            print("Unsupported reg type")
            exit()
        fields = reg.get("fields")
        
        if fields is not None:
            output_fields.append("")
            for field in fields:
                field_name = field["name"]
                field_size = reg.get("size")

                if field_size is None: 
                    field_size = 1  # default to field size of 1 if not provided in yaml

                field_pos = field["pos"]
                line = tab_size * " " + f"using {field_name} = field_t<&{struct_name}::{name}, {field_pos}"
                if (field_size != 1):
                    line += f", {field_size}"  
                line += ">;"

                output_fields.append(line)
            last_had_fields = True
        else:
            # no fields provided

            if last_had_fields == True:
                output_fields.append("") # this block is just to make things look nice
                last_had_fields = False
            if typ[0] == "float":
                output_fields.append(tab_size * " " + f"using {name.lower()} = field_t<&{struct_name}::{name}>;")
            else:
                output_fields.append(tab_size * " " + f"using {name.lower()} = field_t<&{struct_name}::{name}, 0, {typ[1]*8}>;")

        output_regs.append(tab_size * " " + f"reg_t<{typ[0]}> {name}{{{current_pos:#x}}};")

        current_pos += typ[1]
        # check that config will fit into last page
        if current_pos >= mem[2]:
            print("Config does not fit in one page")
            exit(1)

    output_regs.append("")
    output_lines += output_regs + output_fields
    output_lines.append("")

    # non-const all function
    output_lines.append(tab_size * " " + "constexpr auto all() {")
    output_lines.append(tab_size * 2 * " " + "return std::forward_as_tuple(")
    # use textwrap to join reg names to fit on multiple lines
    joined_names = ", ".join(names)
    wrapped = textwrap.fill(
        joined_names,
        width = 80,
        initial_indent=" " * tab_size*3,
        subsequent_indent=" " * tab_size*3
    )
    output_lines.append(wrapped)
    output_lines.append(tab_size * 2 * " " + ");")
    output_lines.append(tab_size * " " + "}\n")

    # const all function
    output_lines.append(tab_size * " " + "constexpr auto all () const {")
    output_lines.append(tab_size * 2 * " " + "return std::forward_as_tuple(")
    output_lines.append(wrapped)
    output_lines.append(tab_size * " " + ");")
    output_lines.append(tab_size * " " + "}\n")

    # memory layout struct
    output_lines.append(tab_size * " " + "struct mem_layout {")
    output_lines.append(tab_size * 2 * " " + f"static constexpr uint32_t FLASH_BEGIN_ADDR = {mem[0]:#x};")
    output_lines.append(tab_size * 2 * " " + f"static constexpr uint32_t FLASH_END_ADDR = {mem[1]:#x};")
    output_lines.append(tab_size * 2 * " " + f"static constexpr int PAGE_SIZE = {mem[2]};")
    output_lines.append(tab_size * 2 * " " + f"static constexpr int NUM_PAGES = {mem[3]};")
    output_lines.append(tab_size * " " + "};\n")

    # set_raw function
    set_raw_fn = """\
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
    output_lines.append(set_raw_fn)

    # get_raw function
    get_raw_fn = """
    auto get_raw(uint8_t address, uint32_t& raw) const -> bool {
        bool found = false;
        std::apply([&](auto const&... reg) -> void {
            ((reg.addr == address ? (raw = to_raw(reg.value.value_or(0)), found = true) : false), ...);
        },
                    all());
        return found;
    }\n"""
    output_lines.append(get_raw_fn)

    # size_bytes function
    size_bytes_fn = """\
    static consteval auto size_bytes() -> uint16_t {
        return validated_config_t<bmc_config_t>::size_bytes();
    }\n"""
    output_lines.append(size_bytes_fn)

    output_lines.append("};\n")

    output_lines.append(f"inline auto get_can_options({struct_name}* config) -> FDCAN::Options {{")

    if (config["can_filtering"] is not None):
        can_reg = config["can_filtering"]["id_reg"] 
        output_lines.append(tab_size * " " + f"config->can_node_filters[0].id1 =  config->get<{struct_name}::{can_reg.lower()}()>;")
        output_lines.append(tab_size * " " + "config->can_node_filters[0].id2 = CAN_DEST_ID_MASK;")
        dest_id_type = can_id_types.get(config["can_filtering"]["id_type"], 0)
        if(dest_id_type == 0):
            print("Unsupported can id type")
            exit()
        output_lines.append(tab_size * " " + f"config->can_node_filters[0].id_type = FDCAN::FilterIdType::{dest_id_type};")
        output_lines.append(tab_size * " " + "config->can_node_filters[0].action = FDCAN::ActionType::Accept;")
        output_lines.append(tab_size * " " + "config->can_node_filters[0].mode = FDCAN::ActionType::Mask;\n")

        num_subs = 0
        if(config["can_filtering"]["can_subs"] is not None):
            num_subs = len(config["can_filtering"]["can_subs"])
            sub_num = 1
            for filter in config["can_filtering"]["can_subs"]:    
                output_lines.append(tab_size * " " + f"config->can_node_filters[{sub_num}].id1 = {filter["id"]};")
                output_lines.append(tab_size * " " + f"config->can_node_filters[{sub_num}].id2 = CAN_SRC_ID_MASK;")
                src_id_type = can_id_types.get(filter["type"], 0)
                if(src_id_type == 0):
                    print("Unsupported can id type")
                    exit()
                output_lines.append(tab_size * " " + f"config->can_node_filters[{sub_num}].id_type = FDCAN::FilterIdType::{src_id_type};")
                output_lines.append(tab_size * " " + f"config->can_node_filters[{sub_num}].action = FDCAN::ActionType::Accept;")
                output_lines.append(tab_size * " " + f"config->can_node_filters[{sub_num}].mode = FDCAN::ActionType::Mask;\n")  
                sub_num += 1
   
        output_lines.append(tab_size * " " + "FDCAN::FilterConfig filter;")
        output_lines.append(tab_size * " " + "filter.begin = config->can_node_filters;")
        output_lines.append(tab_size * " " + f"filter.end = config->can_node_filters + {num_subs + 1}")
        output_lines.append(tab_size * " " + "filter.global_non_matching_std_action = FDCAN::FilterAction::Reject;")
        output_lines.append(tab_size * " " + "filter.global_non_matching_ext_action = FDCAN::FilterAction::Reject;\n")

        output_lines.append(tab_size * " " + "auto can_opts = FDCAN::Options{};")
        output_lines.append(tab_size * " " + "can_opts.delay_compensation = true;")
        output_lines.append(tab_size * " " + "can_opts.tdc_offset = 13;")
        output_lines.append(tab_size * " " + "can_opts.tdc_filter = 1;")
        output_lines.append(tab_size * " " + "can_opts.filter_config = filter;")
        output_lines.append(tab_size * " " + "return can_opts;\n")
        output_lines.append("}\n")

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
        type=argparse.FileType('w'),
        required=True,
        help="Path to file where config will be generated",
    )
    parser.add_argument(
        "--tabs",
        type=int,
        required=False,
        default=4,
        help="Tab size in output file",
    )

    args = parser.parse_args()

    print(generate_config_struct(args.input, args.tabs), file=args.output)