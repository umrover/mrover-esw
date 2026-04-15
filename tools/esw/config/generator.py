# user imports
from .types import ChipInfo, RegGenResult, TypeInfo, chips, types, can_id_types

# libraries
from pathlib import Path
import yaml
from datetime import datetime
from jinja2 import Environment, FileSystemLoader, StrictUndefined


class ConfigGen:
    project_name: str
    struct_name: str | None
    tab_size: int

    def __init__(self, project_name: str, tab_size: int, template_dir: Path):
        self.tab_size = tab_size
        self.project_name = project_name
        self.env = Environment(
            loader=FileSystemLoader(str(template_dir)),
            undefined=StrictUndefined,
            trim_blocks=True,
            lstrip_blocks=True,
        )

    def generate_config_header(self, yaml_path: str | Path, output_path: str | Path) -> None:
        with open(yaml_path) as file:
            config = yaml.safe_load(file)
        self.struct_name = config.get("struct_name")
        if self.struct_name is None:
            raise ValueError(
                "Missing required field 'struct_name' in config. 'struct_name' defines the name of the config struct to generate"
            )

        regs: list[dict] | None = config.get("regs")
        if regs is None:
            raise ValueError(
                "Missing required field 'regs' in config. 'regs' defines the list of registers to generate."
            )

        chip: str | None = config.get("chip")
        if chip is None:
            raise ValueError(f"Missing required field 'chip' in config. Must be one of: {list(chips.keys())}")
        mem: ChipInfo | None = chips.get(chip)
        if mem is None:
            raise ValueError(f"Unsupported chip type: {config.get('chip')}. Must be one of: {list(chips.keys())}")

        result: RegGenResult = self.generate_regs_fields(regs, mem)
        reg_names: list[str] = result.reg_names

        if config.get("can_filtering") is None:
            raise ValueError("can_filtering not specified, unable to generate get_can_options")

        self.validate_can_filtering(config["can_filtering"], reg_names)

        template = self.env.get_template("config_header.hpp.j2")

        with open(output_path, "w") as f:
            template.stream(
                timestamp=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                project_name=self.project_name,
                struct_name=self.struct_name,
                reg_output=result.reg_output,
                field_output=result.field_output,
                reg_names=reg_names,
                flash_begin=f"{mem.flash_begin:#x}",
                flash_end=f"{mem.flash_end:#x}",
                flash_page_size=mem.page_size,
                flash_num_pages=mem.num_pages,
                can_filtering=config["can_filtering"],
            ).dump(f)

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
                raise ValueError(f"Unsupported reg type: {reg['type']}. Must be one of {list(types.keys())}")
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

    def validate_can_filtering(self, can_filtering: dict, reg_names: list[str]) -> None:

        can_reg: str | None = can_filtering.get("id_reg")
        if can_reg is None:
            raise ValueError(
                "Missing required field 'id_reg' in can, should specify name of register containing can id"
            )
        if can_reg.upper() not in reg_names:
            raise ValueError(f"id_reg is not a defined register, value: {can_reg}")

        dest_id_type: str | None = can_filtering.get("id_type")
        if dest_id_type is None:
            raise ValueError("Missing required field 'id_type' in can, should specify can id type")

        resolved_dest_id_type = can_id_types.get(dest_id_type)
        if resolved_dest_id_type is None:
            raise ValueError(f"Unsuported CAN id type: {can_filtering['id_type']}")

        if can_filtering.get("delay_compensation") is None:
            raise ValueError("Missing required field 'delay_compensation' in can")
        if can_filtering.get("tdc_offset") is None:
            raise ValueError("Missing required field 'tdc_offset' in can")
        if can_filtering.get("tdc_filter") is None:
            raise ValueError("Missing required field 'tdc_filter' in can")

        if can_filtering.get("can_subs") is not None:
            for sub in can_filtering["can_subs"]:
                src_id: int | None = sub.get("can_id")
                if src_id is None:
                    raise ValueError(
                        "Missing required field 'can_id' in a can sub, should specify should specify can id to subscribe to"
                    )

                src_id_type: str | None = sub.get("id_type")
                if src_id_type is None:
                    raise ValueError(
                        "Missing required field 'id_type' in a can sub, should specify should specify can id type"
                    )

                resolved_src_id_type = can_id_types.get(src_id_type)
                if resolved_src_id_type is None:
                    raise ValueError(f"Unsuported CAN id type: {sub['type']}")
