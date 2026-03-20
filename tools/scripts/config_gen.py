import textwrap
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

def generate_config_struct(yaml_path: str) -> str:
    with open(yaml_path) as file:
        config = yaml.safe_load(file)
    struct_name = config["name"]
    names = []
    regs = config["regs"]
    mem = chips[config["chip"]]

    output_lines = [f"struct {struct_name} {{\n"]
    output_lines.append("    static inline void* flash_ptr = nullptr;\n")
    output_lines.append("    FDCAN::Filter can_node_filter{};\n")

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
                line = f"    using {field_name} = field_t<&{struct_name}::{name}, {field_pos}"
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
                output_fields.append(f"    using {name.lower()} = field_t<&{struct_name}::{name}>;")
            else:
                output_fields.append(f"    using {name.lower()} = field_t<&{struct_name}::{name}, 0, {typ[1]*8}>;")

        output_regs.append(f"    reg_t<{typ[0]}> {name}{{{current_pos:#x}}};")

        current_pos += typ[1]
        # check that config will fit into last page
        if current_pos >= mem[2]:
            print("Config does not fit in one page")
            exit(1)

    output_regs.append("")
    output_lines += output_regs + output_fields
    output_lines.append("")

    # non-const all function
    output_lines.append("    constexpr auto all() {")
    output_lines.append("        return std::forward_as_tuple(")
    # use textwrap to join reg names to fit on multiple lines
    joined_names = ", ".join(names)
    wrapped = textwrap.fill(
        joined_names,
        width = 80,
        initial_indent=" " * 12,
        subsequent_indent=" " * 12
    )
    output_lines.append(wrapped)
    output_lines.append("        );")
    output_lines.append("    }\n")

    # const all function
    output_lines.append("    constexpr auto all () const {")
    output_lines.append("        return std::forward_as_tuple(")
    output_lines.append(wrapped)
    output_lines.append("        );")
    output_lines.append("    }\n")

    # memory layout struct
    output_lines.append("    struct mem_layout {")
    output_lines.append(f"        static constexpr uint32_t FLASH_BEGIN_ADDR = {mem[0]:#x};")
    output_lines.append(f"        static constexpr uint32_t FLASH_END_ADDR = {mem[1]:#x};")
    output_lines.append(f"        static constexpr int PAGE_SIZE = {mem[2]};")
    output_lines.append(f"        static constexpr int NUM_PAGES = {mem[3]};")
    output_lines.append("    };\n")

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

    output_lines.append("};")

    return "\n".join(output_lines)

if __name__ == "__main__":
    print(generate_config_struct("configs/new_bmc.yaml"))