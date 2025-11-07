import os
import re

# Class Objects
class DBC:
    name: str
    def __init__(self, name):
        self.name = name
        self.message_dict = {}

class Message:
    name: str
    byte_length: int
    def __init__(self, name):
        self.name = name
        self.bit_length = 0
        self.signal_dict = {}
    
class Signal:
    data_type: str
    start_bit: int
    bit_length: int
    byte_length: int
    def __init__(self, data_type, start_bit, bit_length):
        self.data_type = data_type
        self.start_bit = start_bit
        self.bit_length = bit_length
        self.byte_length = bit_length // 8

# TODO: add some type hints

def update_type(line, dbc_file):
    msg_id = line[1]
    msg = dbc_file.message_dict[msg_id]
    sig_name = line[2]
    sig = msg.signal_dict[sig_name]
    if line[4][0:-1] == "1":
        sig.data_type = "float"
    elif line[4][0:-1] == "2":
        sig.data_type = "double"

def parse_message(line, dbc_file):

    id = line[1]
    name = line[2][:-1]
    byte_length = line[3]
    sender = line[4]

    # TODO: make parse_signal() a helper for parse_message
    # if int(byte_length):
    msg = Message(name)
    # Append to most recent dbc object's message dict
    dbc_file.message_dict[id] = msg

# TODO: order the message signals by size (descending)
# Currently we assume they are already ordered :)
def parse_signal(line, dbc_file):
    name = line[1]

    bits_section = re.split(r'[|@]', line[3])
    start_bit = bits_section[0]
    bit_length = int(bits_section[1])
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
        if isSigned:
            data_type = "int"
        else:
            data_type = "uint"
        data_type += str(bit_length) + "_t"

    # TODO: if supporting scale check scale to see if it's a float
    sig = Signal(data_type, start_bit, bit_length)

    msg_id = list(dbc_file.message_dict.keys())[-1]
    msg = dbc_file.message_dict[msg_id]
    msg.signal_dict[name] = sig

    # Update size of message
    msg.bit_length += bit_length

def parse_file(filepath):
    dbc_file = DBC(os.path.basename(filepath)[:-4])
    ns_flag = False
    with open(filepath) as file:
        for line in file:
            trimmed_line = line.strip()
            split_line = trimmed_line.split(" ")
            if ns_flag or split_line[0] == "NS_":
                if split_line[0] == "NS_" or split_line[0] == "":
                    ns_flag = not ns_flag
            elif split_line[0] == "BO_":
                parse_message(split_line, dbc_file)
            elif split_line[0] == "SG_":
                parse_signal(split_line, dbc_file)
            elif split_line[0] == "SIG_VALTYPE_":
                update_type(split_line, dbc_file)

        # Once all messages are parsed, get byte values
        for msg in dbc_file.message_dict.values():
            # Use bit length to get byte length
            msg.byte_length = msg.bit_length // 8
            msg.byte_length += 1 if msg.bit_length % 8 else 0
        return dbc_file