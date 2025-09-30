import dbc_parser

output = ""

# Include libraries
def gen_libs():
    global output
    libraries = "#include <cstdint>\n\n"
    output += libraries

# Generate MSG structs
def gen_struct(msg):
    global output
    struct = "struct " + msg.name + " {\n"
    sig_list = list(msg.signal_dict.keys())
    for sig_name in sig_list:
        sig = msg.signal_dict[sig_name]
        struct += "\t" + sig.data_type + " " + sig_name + ";\n"
    struct += "};\n\n"
    output += struct


def gen_structs():
    msg_list = list(dbc_parser.message_dict.values())

    for msg in msg_list:
        gen_struct(msg)

# Generate Encoding Func

def gen_encode(msg):
    global output

    # Generate function header
    funct = "uint8_t* " + msg.name + "_encode(" + msg.name + " msg) {\n"
    
    # Determine array length
    

    output += funct

# Generate Decoding Func
gen_libs()
gen_structs()

with open("can_science_test.hpp", "w") as file:
    file.write(output)