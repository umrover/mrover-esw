import argparse
from pathlib import Path
from jinja2 import Environment, FileSystemLoader

from esw.dbc.parser import parse_file


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Parse dbc files to generate C header files.")
    parser.add_argument(
        "files",
        nargs="+", # One or more files
        type=str,
        help="List of dbc files"
    )
    parser.add_argument(
        "--dest",
        "-d",
        type=Path,
        required=True,
        help="Path to generated header file directory",
    )
    parser.add_argument(
        "--ctx",
        "-c",
        type=Path,
        required=True,
        help="Path DBC build context",
    )
    args = parser.parse_args()

    env = Environment(loader=FileSystemLoader(args.ctx))
    template = env.get_template("templates/dbc_header.h.j2")

    libs = ["cstdlib", "cstdint", "bit"]

    for f in args.files:
        # Call parser.py to parse dbc file
        dbc = parse_file(f)

        # Declare data object to pass into jinja2 template
        data = {
            "dbc_name": dbc.name,
            "libs": libs,
            "message_dict": dbc.message_dict
        }

        # Pass message_dict and library list
        rendered = template.render(data)

        # Write to new header file
        if not args.dest.exists():
            args.dest.mkdir(parents=True, exist_ok=True)
        header_file = args.dest / f"{dbc.name}.h"
        with open(header_file, "w") as handle:
            handle.write(rendered)