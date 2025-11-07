import dbc_parser
import argparse
from jinja2 import Environment, FileSystemLoader

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Parse dbc files to generate C header files.")
    parser.add_argument(
        "files",
        nargs="+", # One or more files
        type=str,
        help="List of dbc files"
    )
    args = parser.parse_args()

    env = Environment(loader=FileSystemLoader("."))
    template = env.get_template("templates/dbc_header.h.j2")

    libs = ["cstdlib", "cstdint", "bit"]

    for f in args.files:
        # Call dbc_parser.py to parse dbc file
        dbc = dbc_parser.parse_file(f)

        # Declare data object to pass into jinja2 template
        data = {
            "dbc_name": dbc.name,
            "libs": libs,
            "message_dict": dbc.message_dict
        }

        # Pass message_dict and library list
        rendered = template.render(data)

        # Write to new header file
        with open(f"gen/{dbc.name}.h", "w") as f:
            f.write(rendered)