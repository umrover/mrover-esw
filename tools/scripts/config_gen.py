import yaml

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
        typ = reg["type"]
        size = reg["size"]
        fields = reg.get("fields")

        if fields is not None:
            for field in fields:
                field_name = field["name"]
                field_size = field["size"]
                field_pos = field["pos"]
                output_fields.append(f"    using {field_name} = field_t<&{struct_name}::{name}, {field_pos}, {field_size}>;")
        else:
            output_fields.append(f"    using {name.lower()} = field_t<&{struct_name}::{name}>;")

        output_regs.append(f"    reg_t<{typ}> {name}{{{current_pos:#x}}};")

        current_pos += size

    output_lines += output_regs + output_fields

    output_lines.append("};")

    return "\n".join(output_lines)

if __name__ == "__main__":
    print(generate_config_struct("configs/test.yaml"))