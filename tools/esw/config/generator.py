from pathlib import Path
import yaml
from datetime import datetime
from jinja2 import Environment, FileSystemLoader, StrictUndefined

from esw.config.types import ChipInfo, TypeInfo, chips, types, can_id_types


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

        current_pos: int = 0
        reg_names: list[str] = []

        for reg in regs:
            name: str = reg["name"].upper()
            reg_names.append(name)

            typ: TypeInfo | None = types.get(reg["type"])
            if typ is None:
                raise ValueError(f"Unsupported reg type: {reg['type']}. Must be one of {list(types.keys())}")

            reg["cpp_type"] = typ.type
            reg["size_bytes"] = typ.size
            reg["address"] = current_pos
            current_pos += typ.size
            reg.setdefault("fields", [])

        if current_pos >= mem.page_size:
            raise ValueError(f"Config does not fit in one page: size {current_pos} exceeds {mem.page_size}")

        if config.get("can_filtering") is None:
            raise ValueError("can_filtering not specified, unable to generate get_can_options")

        self.validate_can_filtering(config["can_filtering"], reg_names)

        template = self.env.get_template("config_header.hpp.j2")

        rendered = template.render(
            timestamp=datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            project_name=self.project_name,
            struct_name=self.struct_name,
            regs=regs,
            reg_names=reg_names,
            flash_begin=f"{mem.flash_begin:#x}",
            flash_end=f"{mem.flash_end:#x}",
            flash_page_size=mem.page_size,
            flash_num_pages=mem.num_pages,
            can_filtering=config["can_filtering"],
        )

        with open(output_path, "w", encoding="utf-8") as f:
            f.write(rendered)

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
        can_filtering["id_type"] = resolved_dest_id_type

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
