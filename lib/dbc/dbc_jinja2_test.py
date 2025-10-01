import dbc_parser
from jinja2 import Environment, FileSystemLoader

env = Environment(loader=FileSystemLoader("."))
template = env.get_template("dbc_header.h.j2")

libs = ["cstdlib"]
data = {
    "libs": libs,
    "messages": dbc_parser.message_dict
}

# Pass message_dict and library list
rendered = template.render(data)

# Write to new header file
with open("dbc_header.h", "w") as f:
    f.write(rendered)