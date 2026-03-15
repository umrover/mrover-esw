import yaml

types = {
    "int" : (int, 4),
    "uint32_t" : ("uint32_t", 4),
    "uint16_t": ("uint16_t", 2),
    "uint8_t" : ("uint8_t", 1),
    "float": ("float", 4),
    "bool": ("bool", 1)
}

def generate_config_struct(yaml_path: str) -> str:
    with open(yaml_path) as file:
        config = yaml.safe_load(file)
    struct_name = config["name"]
    regs = config["regs"]

    output_lines = [f"struct {struct_name} {{"]

    output_regs = []
    output_fields = []

    current_pos = 0

    for reg in regs:
        name = reg["name"].upper()
        typ = types.get(reg["type"], 0)
        if (typ == 0):
            print("Unsupported reg type")
            exit()
        fields = reg.get("fields")
        

        if fields is not None:
            for field in fields:
                field_name = field["name"]
                field_size = field["size"]
                field_pos = field["pos"]
                output_fields.append(f"    using {field_name} = field_t<&{struct_name}::{name}, {field_pos}, {field_size}>;")
        else:
            output_fields.append(f"    using {name.lower()} = field_t<&{struct_name}::{name}>;")

        output_regs.append(f"    reg_t<{typ[0]}> {name}{{{current_pos:#x}}};")

        current_pos += typ[1]

    output_lines += output_regs + output_fields

    output_lines.append("};")

    return "\n".join(output_lines)

if __name__ == "__main__":
    print(generate_config_struct("configs/test.yaml"))