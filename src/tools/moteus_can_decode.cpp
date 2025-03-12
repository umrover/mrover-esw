// To compile:
// g++ moteus_can_decode.cpp -Wall -Werror -o moteus_can_decode
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

enum class Subframe : uint8_t {
    // # Register RPC #
    kWriteInt8 = 0x00,
    kWriteInt16 = 0x04,
    kWriteInt32 = 0x08,
    kWriteFloat = 0x0c,

    kReadInt8 = 0x10,
    kReadInt16 = 0x14,
    kReadInt32 = 0x18,
    kReadFloat = 0x1c,

    kReplyInt8 = 0x20,
    kReplyInt16 = 0x24,
    kReplyInt32 = 0x28,
    kReplyFloat = 0x2c,

    kWriteError = 0x30,
    kReadError = 0x31,

    kNop = 0x50,
};

enum class RegType {
    Mode,
    Position,
    Velocity,
    Acceleration,
    Torque,
    Current,
    Voltage,
    Power,
    Temperature,
    Trajectory,
    HomeState,
    FaultCode,
    PWMGain, // PWM or Kp/Kd
    Time,
    Validity, // Bitfield
    GPIO,
    AnalogInput,
    ClockTrim,
    Number,
    FWVersion
};

struct Reg {
    uint16_t num;
    std::string name;
    RegType type;
};

std::vector<Reg> reg_map = {
    { 0x0, "Mode", RegType::Mode },
    { 0x1, "Servo position", RegType::Position },
    { 0x2, "Servo velocity", RegType::Velocity },
    { 0x3, "Torque", RegType::Torque },
    { 0x4, "Measured Q Phase Current", RegType::Current },
    { 0x5, "Measured D Phase Current", RegType::Current },
    { 0x6, "Absolution position", RegType::Position },
    { 0x7, "Measured power", RegType::Power },
    { 0xa, "Motor temperature", RegType::Temperature },
    { 0xb, "Trajectory Complete", RegType::Trajectory },
    { 0xc, "Home state", RegType::HomeState },
    { 0xd, "Input voltage", RegType::Voltage },
    { 0xe, "Board temperature", RegType::Temperature },
    { 0xf, "Fault code", RegType::FaultCode },
    { 0x10, "PWM Phase A", RegType::PWMGain },
    { 0x11, "PWM Phase B", RegType::PWMGain },
    { 0x12, "PWM Phase C", RegType::PWMGain },
    { 0x14, "Voltage Phase A", RegType::Voltage },
    { 0x15, "Voltage Phase B", RegType::Voltage },
    { 0x16, "Voltage Phase C", RegType::Voltage },
    { 0x18, "Voltage FOC Mode: desired electrical phase", RegType::PWMGain },
    { 0x19, "Voltage FOC Mode: desired applied phase voltage", RegType::Voltage },
    { 0x1a, "D Voltage", RegType::Voltage },
    { 0x1b, "Q Voltage", RegType::Voltage },
    { 0x1c, "Commanded Q Phase current", RegType::Current },
    { 0x1d, "Commanded D Phase current", RegType::Current },
    { 0x1e, "Voltage FOC Mode: theta rate", RegType::Velocity },
    { 0x20, "Desired position", RegType::Position },
    { 0x21, "Desired velocity", RegType::Velocity },
    { 0x22, "Feedforward torque", RegType::Torque },
    { 0x23, "Kp scale", RegType::PWMGain },
    { 0x24, "Kd scale", RegType::PWMGain },
    { 0x25, "Maximum torque", RegType::Torque },
    { 0x26, "Desired stop position", RegType::Position },
    { 0x27, "Watchdog Timeout", RegType::Time },
    { 0x28, "Velocity Limit", RegType::Velocity },
    { 0x29, "Acceleration Limit", RegType::Acceleration },
    { 0x2a, "Fixed Voltage override", RegType::Number },
    { 0x2b, "Ki integral windup limit factor", RegType::PWMGain },
    { 0x30, "Kp torque", RegType::Torque },
    { 0x31, "Ki torque", RegType::Torque },
    { 0x32, "Kd torque", RegType::Torque },
    { 0x33, "PID Feedforward torque", RegType::Torque },
    { 0x34, "Position control commanded torque", RegType::Torque },
    { 0x38, "Trajectory position", RegType::Position },
    { 0x39, "Trajectory velocity", RegType::Velocity },
    { 0x3a, "Trajectory torque", RegType::Torque },
    { 0x3b, "Position error", RegType::Position },
    { 0x3c, "Velocity error", RegType::Velocity },
    { 0x3d, "Torque error", RegType::Torque },
    { 0x40, "Minimum position", RegType::Position },
    { 0x41, "Maximum position", RegType::Position },
    { 0x42, "Feedforward torque", RegType::Torque },
    { 0x43, "Kp scale", RegType::PWMGain },
    { 0x44, "Kd scale", RegType::PWMGain },
    { 0x45, "Maximum torque", RegType::Torque },
    { 0x46, "Watchdog Timeout", RegType::Time },
    { 0x47, "Ki integral windup limit factor", RegType::PWMGain },
    { 0x50, "Encoder 0 position", RegType::Position },
    { 0x51, "Encoder 0 velocity", RegType::Velocity },
    { 0x52, "Encoder 1 position", RegType::Position },
    { 0x53, "Encoder 1 velocity", RegType::Velocity },
    { 0x54, "Encoder 2 position", RegType::Position },
    { 0x55, "Encoder 2 velocity", RegType::Velocity },
    { 0x58, "Encoder validity", RegType::Validity },
    { 0x5c, "Aux1 GPIO Command", RegType::GPIO },
    { 0x5d, "Aux2 GPIO Command", RegType::GPIO },
    { 0x5e, "Aux1 GPIO Status", RegType::GPIO },
    { 0x5f, "Aux2 GPIO Status", RegType::GPIO },
    { 0x60, "Aux1 Analog Inputs", RegType::AnalogInput },
    { 0x64, "Aux1 Analog Inputs", RegType::AnalogInput },
    { 0x68, "Aux2 Analog Inputs", RegType::AnalogInput },
    { 0x6c, "Aux2 Analog Inputs", RegType::AnalogInput },
    { 0x70, "Millisecond counter", RegType::Number },
    { 0x71, "Clock Trim", RegType::Number },
    { 0x76, "Aux1 PWM Outputs", RegType::PWMGain },
    { 0x7a, "Aux1 PWM Outputs", RegType::PWMGain },
    { 0x7b, "Aux2 PWM Outputs", RegType::PWMGain },
    { 0x7f, "Aux2 PWM Outputs", RegType::PWMGain },
    { 0x100, "Model Number", RegType::Number },
    { 0x101, "Firmware Version", RegType::FWVersion },
    { 0x102, "Register Map Version", RegType::Number },
    { 0x110, "Multiplex ID", RegType::Number },
    { 0x120, "Serial Number", RegType::Number },
    { 0x121, "Serial Number", RegType::Number },
    { 0x122, "Serial Number", RegType::Number },
    { 0x130, "Set Output Nearest", RegType::Number },
    { 0x131, "Set Output Exact", RegType::Number },
    { 0x132, "Require Reindex", RegType::Number },
    { 0x140, "Driver Fault 1", RegType::Number },
    { 0x141, "Driver Fault 2", RegType::Number },
};

enum class MSGPart {
    Command,
    RegisterNum,
    RegisterValue,
    ErrorValue
};

enum class CMDType {
    Write,
    Read,
    Reply,
    Error,
    Nop
};

enum class RegLength {
    Reg8,
    Reg16,
    Reg32,
    RegFloat
};

struct CMDInfo {
    uint8_t num_regs;
    CMDType type;
    RegLength reg_length;
    bool used_num_byte = false;
};

CMDInfo parse_cmd_type(uint8_t cmd_byte, uint8_t next_byte)
{
    uint8_t num_regs = cmd_byte & 0x3;
    RegLength reg_length = static_cast<RegLength>((cmd_byte >> 2) & 0x3);
    CMDType cmd_type = static_cast<CMDType>((cmd_byte >> 4) & 0xF);
    std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(cmd_byte) << ": ";

    CMDInfo cmd_info;
    cmd_info.type = cmd_type;
    cmd_info.reg_length = reg_length;

    if (num_regs == 0) {
        num_regs = next_byte;
        if (cmd_type != CMDType::Nop && cmd_type != CMDType::Error)
            cmd_info.used_num_byte = true;
    }
    cmd_info.num_regs = num_regs;

    switch (cmd_type) {
    case CMDType::Write:
        std::cout << "write ";
        break;
    case CMDType::Read:
        std::cout << "read ";
        break;
    case CMDType::Reply:
        std::cout << "reply ";
        break;
    case CMDType::Error:
        std::cout << "error ";
        break;
    case CMDType::Nop:
        std::cout << "NOP\n";
        return cmd_info;
        break;
    }
    std::cout << static_cast<int>(num_regs) << " x ";

    switch (reg_length) {
    case RegLength::Reg8:
        std::cout << "INT8s\n";
        break;
    case RegLength::Reg16:
        std::cout << "INT16s\n";
        break;
    case RegLength::Reg32:
        std::cout << "INT32s\n";
        break;
    case RegLength::RegFloat:
        std::cout << "FLOATs\n";
        break;

    default:
        break;
    }

    return cmd_info;
}

void parse_value(int value, RegType type, RegLength length)
{
    switch (type) {
    case RegType::Mode:
        switch (value) {
        case 0:
            std::cout << "Stopped (writeable, clears faults)\n";
            break;

        case 1:
            std::cout << "Fault\n";
            break;

        case 2:
        case 3:
        case 4:
            std::cout << "Preparing to Operate\n";
            break;

        case 5:
            std::cout << "PWM Mode\n";
            break;

        case 6:
            std::cout << "Voltage Mode\n";
            break;

        case 7:
            std::cout << "Voltage FOC\n";
            break;

        case 8:
            std::cout << "Voltage DQ\n";
            break;

        case 9:
            std::cout << "Current Mode\n";
            break;

        case 10:
            std::cout << "Position Mode\n";
            break;

        case 11:
            std::cout << "Timeout\n";
            break;

        case 12:
            std::cout << "Zero Velocity\n";
            break;

        case 13:
            std::cout << "Stay Within\n";
            break;

        case 14:
            std::cout << "Measure Inductance\n";
            break;

        case 15:
            std::cout << "Brake\n";
            break;

        default:
            std::cout << "Unknown Mode\n";
            break;
        }

        break;
    case RegType::Position:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 3.6; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.036;
            break;
        case RegLength::Reg32:

            std::cout << (float)value * 0.00036;
            break;
        case RegLength::RegFloat: {
            float* aah = (float*)(&value);
            std::cout << *aah;
            break;
        }

        default:
            break;
        }
        std::cout << " deg \n";
        break;

    case RegType::Velocity:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 36; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.09;
            break;
        case RegLength::Reg32:

            std::cout << (float)value * 0.0036;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " dps \n";
        break;

    case RegType::Acceleration:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 0.05; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.001;
            break;
        case RegLength::Reg32:
            std::cout << (float)value * 0.00001;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " I/s^2 \n";
        break;

    case RegType::Torque:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 0.5; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.01;
            break;
        case RegLength::Reg32:
            std::cout << (float)value * 0.001;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " Nm \n";
        break;

    case RegType::Current:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 1; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.1;
            break;
        case RegLength::Reg32:
            std::cout << (float)value * 0.001;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " A \n";
        break;

    case RegType::Voltage:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 0.5; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.1;
            break;
        case RegLength::Reg32:
            std::cout << (float)value * 0.001;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " V\n";
        break;

    case RegType::Power:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 10.0; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.05;
            break;
        case RegLength::Reg32:
            std::cout << (float)value * 0.0001;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " W \n";
        break;
    case RegType::Temperature:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 1; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.1;
            break;
        case RegLength::Reg32:
            std::cout << (float)value * 0.001;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " C \n"; // This is how you know a European wrote this code
        break;

    case RegType::Trajectory:
        if (value == 0) {
            std::cout << "Trajectory incomplete\n";
        } else {
            std::cout << "Trajectory complete\n";
        }
        break;

    case RegType::HomeState:
        if (value == 0) {
            std::cout << "relative only\n";
        } else if (value == 1) {
            std::cout << "rotor\n";
        } else {
            std::cout << "output\n";
        }
        break;

    case RegType::FaultCode:
        std::cout << value << "\n";
        break;

    case RegType::PWMGain: // PWM or Kp/Kd
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * (1.0 / 127.0); // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * (1.0 / 32767.0);
            break;
        case RegLength::Reg32:
            std::cout << (float)value * (1.0 / 2147483647.0);
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << "\n";
        break;

    case RegType::Time:
        switch (length) {
        case RegLength::Reg8:
            std::cout << (float)value * 0.01; // Do degrees for now
            break;
        case RegLength::Reg16:
            std::cout << (float)value * 0.0001;
            break;
        case RegLength::Reg32:
            std::cout << (float)value * 0.00001;
            break;
            case RegLength::RegFloat: {
                float* aah = (float*)(&value);
                std::cout << *aah;
                break;
            }
        default:
            break;
        }
        std::cout << " s\n";
        break;

    case RegType::Validity: // Bitfield
        std::cout << std::bitset<8>(value) << "\n";
        break;
    case RegType::GPIO:
        std::cout << std::bitset<8>(value) << "\n";
        break;

    case RegType::AnalogInput:
        std::cout << value << "\n";
        break;

    case RegType::ClockTrim:
        std::cout << value << "\n";
        break;
    case RegType::Number:
        std::cout << value << "\n";
        break;

    case RegType::FWVersion: {
        uint32_t ver = static_cast<uint32_t>(value);
        std::cout << ((ver >> 16) & 0xff) << '.' << ((ver >> 8) & 0xff) << '.' << (ver & 0xff) << '\n';
        break;
    }
    default:
        // Handle unknown or unsupported type
        break;
    }
}

void parse_reg(uint32_t reg_num, RegLength length, uint32_t reg_value = 0, bool show_value = false)
{
    std::cout << "\t[" << reg_num << "]: ";
    auto reg_info_it = std::find_if(
        reg_map.begin(),
        reg_map.end(),
        [reg_num](const Reg& reg) {
            return reg.num == reg_num;
        });
    if (reg_info_it == reg_map.end())
        throw std::runtime_error("Reg " + std::to_string(reg_num) + " does not exist");

    Reg reg_info = *reg_info_it;
    if (show_value) {
        std::cout << reg_info.name << " -> " << std::hex;
        parse_value(reg_value, reg_info.type, length);
    } else
        std::cout << reg_info.name << "\n";
}

void ParseMoteusCANFrame(uint8_t data[], size_t data_len)
{
    MSGPart msg_part_type = MSGPart::Command;
    CMDInfo cmd_info;
    uint32_t reg_num;
    uint8_t reg_num_part_idx = 0;

    for (size_t i = 0; i < data_len; i++) {
        uint8_t curr_byte = data[i];

        // What our current byte represents
        switch (msg_part_type) {
        case MSGPart::Command: {
            cmd_info = parse_cmd_type(curr_byte, data[i + 1]);
            if (cmd_info.used_num_byte)
                i++;
            msg_part_type = (cmd_info.type == CMDType::Nop) ? MSGPart::Command : MSGPart::RegisterNum;
            reg_num_part_idx = 0;
            break;
        }

        case MSGPart::RegisterNum: {
            uint8_t reg_num_part = curr_byte;
            reg_num = (reg_num_part << (8 * reg_num_part_idx));
            reg_num_part_idx++;

            if (!(reg_num_part & 0x80) || reg_num_part_idx == 4) {
                // Done with register number
                msg_part_type = (cmd_info.type == CMDType::Error) ? MSGPart::ErrorValue : (cmd_info.type == CMDType::Write || cmd_info.type == CMDType::Reply) ? MSGPart::RegisterValue
                                                                                                                                                               : MSGPart::Command;
                if (cmd_info.type == CMDType::Read) {
                    for (size_t reg_num_i = 0; reg_num_i < cmd_info.num_regs; reg_num_i++) {
                        parse_reg(reg_num + reg_num_i, cmd_info.reg_length);
                    }
                }
            }
            break;
        }

        case MSGPart::ErrorValue: {
            std::cout << std::hex << curr_byte << std::dec << "\n";
            msg_part_type = MSGPart::Command;
            break;
        }

        case MSGPart::RegisterValue: {
            int num_bytes = cmd_info.num_regs;
            int jump = 1;
            switch (cmd_info.reg_length) {
            case RegLength::Reg8:
                jump = 1;
                break;

            case RegLength::Reg16:
                jump = 2;
                break;

            case RegLength::Reg32:
            case RegLength::RegFloat:
                jump = 4;
                break;
            }
            num_bytes *= jump;

            for (int k = 0; k < num_bytes; k += jump) {

                int reg_value;
                switch (cmd_info.reg_length) {
                case RegLength::Reg8:
                    reg_value = static_cast<int8_t>(data[i + k]);
                    break;
                // TODO: Handle unsigned numbers.
                case RegLength::Reg16:
                    reg_value = static_cast<int16_t>(data[i + k] | ((uint16_t)data[i + k + 1] << 8));
                    break;

                case RegLength::Reg32:
                case RegLength::RegFloat: // 32 bits
                    reg_value = static_cast<int32_t>(data[i + k] | ((uint32_t)data[i + k + 1] << 8) | ((uint32_t)data[i + k + 2] << 16) | ((uint32_t)data[i + k + 3] << 24));
                    break;

                default:
                    break;
                }
                if (cmd_info.type == CMDType::Write || cmd_info.type == CMDType::Reply)
                    parse_reg(reg_num + (k / jump), cmd_info.reg_length, reg_value, true);
                else if (cmd_info.type == CMDType::Read)
                    parse_reg(reg_num + (k / jump), cmd_info.reg_length);
            }
            i += (num_bytes - 1);
            msg_part_type = MSGPart::Command;
            break;
        }
        }
    }
}

uint8_t HexChar2Num(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    } else if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    } else {
        throw std::runtime_error("Character is not a valid hex character: " + c);
    }
}

void string_to_u8arr(const std::string& input, uint8_t arr[])
{
    for (size_t i = 0; i < input.size() / 2; i++) {
        char Ldigit = input[2 * i];
        char Rdigit = input[2 * i + 1];
        uint8_t Lnum = HexChar2Num(Ldigit);
        uint8_t Rnum = HexChar2Num(Rdigit);
        uint8_t num = (Lnum << 4) | Rnum;
        arr[i] = num;
    }
}

void printHelp() {
    std::cout << 
    "./moteus_can_decode [FRAME | -h | --help]\n"
    "  FRAME: CAN frame in hex\n"
    "  -h/--help: prints this message\n"
    "Prints decoded message in the form:\n"
    "[command byte]: [write/read/reply/error/nop] [Number of registers] x [INT8/INT16/INT32/FLOAT]\n"
    "   [register_num]: [Register description] [--> value]\n"
    "   [register_num]: [Register description] [--> value]\n"
    "   ...\n";
}

int main(int argc, char** argv)
{
    std::string cmd_string;

    if (argc == 1) {
        std::cin >> cmd_string;
    } else {
        cmd_string = argv[1];
    }

    if (cmd_string == "-h" || cmd_string == "--help") {
        printHelp();
        return 0;
    }

    assert(cmd_string.size() % 2 == 0);

    uint8_t* array = new uint8_t[cmd_string.size() / 2];
    string_to_u8arr(cmd_string, array);
    ParseMoteusCANFrame(array, cmd_string.size() / 2);
    delete[] array;
}
