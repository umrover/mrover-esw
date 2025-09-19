import os
import re

# Global variables
message_dict = {}   # Key: message_id

# Class Objects
class message:
    name: str
    byte_length: int
    signal_dict = {}    # Key: signal_id
    def __init__(self, name, byte_length, signal_dict):
        self.name = name
        self.byte_length = byte_length
        self.signal_dict = signal_dict
    
class signal:
    data_type: str
    start_bit: int
    bit_length: int
    def __init__(self, data_type, start_bit, bit_length):
        self.data_type = data_type
        self.start_bit = start_bit
        self.bit_length = bit_length

# TODO: add some type hints

#
def parse_message(line):
    id = line[1]
    name = line[2][:-1]
    byte_length = line[3]
    sender = line[4]

    signal_dict = {}

    msg = message(name, byte_length, signal_dict)

    message_dict[id] = msg

#
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
    if bit_length == 1:
        data_type = "bool"
    else:
        data_type = "int"
    # TODO: if supporting scale check scale to see if it's a float

    sig = signal(data_type, start_bit, bit_length)
    msg_id = list(message_dict.keys())[-1]
    msg = message_dict[msg_id]
    msg.signal_dict[name] = sig

#
def parse_file(file_handle):
    with open(file_handle) as file:
        for line in file:
            trimmed_line = line.strip()
            split_line = trimmed_line.split(" ")
            if split_line[0] == "BO_":
                parse_message(split_line)
            elif split_line[0] == "SG_":
                parse_signal(split_line)
            print(message_dict)
       

# Get path of current script
path = os.path.dirname(os.path.realpath(__file__))
# Look through directory for .dbc files
for root, dirs, files in os.walk(path):
    for file in files:
        # Open the .dbc file
        if file.endswith(".dbc"):
            # Helper function to parse .dbc file
            parse_file(file)