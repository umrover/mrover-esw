import os
import re

# Global variables
dbc_arr = []        # Holds all .dbc files

# Class Objects
class dbc:
    name: str

    def __init__(self, name):
        self.name = name
        self.message_dict = {}

class message:
    name: str
    byte_length: int
    def __init__(self, name, byte_length):
        self.name = name
        self.byte_length = byte_length
        self.signal_dict = {}
        
    
class signal:
    data_type: str
    start_bit: int
    bit_length: int
    def __init__(self, data_type, start_bit, bit_length):
        self.data_type = data_type
        self.start_bit = start_bit
        self.bit_length = bit_length

# TODO: add some type hints

def update_type(line):
    msg_id = line[1]
    msg = dbc_arr[-1].message_dict[msg_id]
    sig_name = line[2]
    sig = msg.signal_dict[sig_name]
    if line[4][0:-1] == "1":
        sig.data_type = "float"
    elif line[4][0:-1] == "2":
        sig.data_type = "double"

def parse_message(line):

    id = line[1]
    name = line[2][:-1]
    byte_length = line[3]
    sender = line[4]

    msg = message(name, byte_length)

    # Append to most recent dbc object's message dict
    dbc_arr[-1].message_dict[id] = msg

def parse_signal(line):
    name = line[1]

    bits_section = re.split(r'[|@]', line[3])
    start_bit = bits_section[0]
    bit_length = bits_section[1]
    isLittleEndian = bool(bits_section[2][0])
    isSigned = True if bits_section[2][1] == "-" else False

    scale_offset_section = line[4][1:-1]
    scale_offset_section = scale_offset_section.split(",")
    scale = scale_offset_section[0]
    offset = scale_offset_section[1]

    min_max_section = line[5][1:-1]
    min_max_section = min_max_section.split("|")
    val_min = min_max_section[0]
    val_max = min_max_section[1]

    unit = line[6][1:-1]
    receiver = line[7]

    # Determine data_type
    data_type = ""
    if bit_length == "1":
        data_type = "bool"
    else:
        if isSigned:
            data_type = "int"
        else:
            data_type = "uint"
        data_type += bit_length + "_t"

    # TODO: if supporting scale check scale to see if it's a float
    sig = signal(data_type, start_bit, bit_length)

    msg_id = list(dbc_arr[-1].message_dict.keys())[-1]
    # print(list(dbc_arr[-1].message_dict.keys())[-1])
    msg = dbc_arr[-1].message_dict[msg_id]
    msg.signal_dict[name] = sig
    # print(f"{msg.signal_dict}\n")

def parse_file(file_handle):
    ns_flag = False
    with open(file_handle) as file:
        for line in file:
            trimmed_line = line.strip()
            split_line = trimmed_line.split(" ")
            if ns_flag or split_line[0] == "NS_":
                if split_line[0] == "NS_" or split_line[0] == "":
                    ns_flag = not ns_flag
            elif split_line[0] == "BO_":
                parse_message(split_line)
            elif split_line[0] == "SG_":
                parse_signal(split_line)
            elif split_line[0] == "SIG_VALTYPE_":
                update_type(split_line)

# The main script code (make it more OOP!)
# Get path of current script
script_path = os.path.dirname(os.path.realpath(__file__))
os.chdir("../../dbc")
dbc_files_path = os.getcwd()

for root, dirs, files in os.walk(dbc_files_path):
    # Go to the dbc/ directory with the .dbc files
    for file in files:
        # Open the .dbc file
        if file.endswith(".dbc"):
            # Go back to original directory
            os.chdir(script_path)

            # Get name of dbc file (remove .dbc extension)
            dbc_name = file[:-4]
            # Add dbc file to dbc array
            dbc_arr.append(dbc(dbc_name))

            # Helper function to parse .dbc file
            parse_file(dbc_files_path + "/" + file)