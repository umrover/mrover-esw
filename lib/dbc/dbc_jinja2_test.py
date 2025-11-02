import dbc_parser
from jinja2 import Environment, FileSystemLoader

env = Environment(loader=FileSystemLoader("."))
template = env.get_template("dbc_header.h.j2")

libs = ["cstdlib", "cstdint", "bit"]

for dbc in dbc_parser.dbc_arr:
    data = {
        "dbc_name": dbc.name,
        "libs": libs,
        "message_dict": dbc.message_dict
    }

    # Pass message_dict and library list
    rendered = template.render(data)

    # Write to new header file
    with open(f"{dbc.name}.h", "w") as f:
        f.write(rendered)