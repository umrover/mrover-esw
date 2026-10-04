#include "main.h"

#include <array>
#include <cstddef>
#include <cstdint>

// CubeMX UART handle for USART1
extern UART_HandleTypeDef huart1;

namespace {

constexpr uint8_t DXL_ID = 1;               // Servo ID
constexpr uint8_t INST_PING = 0x01;         // Ping instr from Protocol 2.0
constexpr uint8_t INST_STATUS = 0x55;       // Status packet instr from Protocol 2.0

// CRC calculation for DYNAMIXEL Protocol 2.0
uint16_t update_crc(uint16_t crc_accum, const uint8_t* data, std::size_t size) {
    for (std::size_t j = 0; j < size; ++j) {
        crc_accum ^= static_cast<uint16_t>(data[j]) << 8;

        for (int i = 0; i < 8; ++i) {
            if (crc_accum & 0x8000) {
                crc_accum = (crc_accum << 1) ^ 0x8005;
            } else {
                crc_accum <<= 1;
            }
        }
    }

    return crc_accum;
}

}  // namespace

volatile HAL_StatusTypeDef tx_result = HAL_OK;
volatile HAL_StatusTypeDef rx_result = HAL_OK;

volatile uint32_t ping_attempts = 0;
volatile uint32_t ping_responses = 0;

// Debug vals from received status packet
volatile bool packet_valid = false;
volatile uint8_t servo_error = 0;
volatile uint16_t model_number = 0;
volatile uint8_t firmware_version = 0;

volatile uint8_t received_data[14]{};

extern "C" void servo_controller_run() {
    // Protocol 2.0 PING to DYNAMIXEL ID 1:
    // FF FF FD 00 | ID | LENGTH_L LENGTH_H | INST | CRC_L CRC_H
    std::array<uint8_t, 10> ping{
        0xFF, 0xFF, 0xFD, 0x00,
        DXL_ID,
        0x03, 0x00,
        INST_PING,
        0x00, 0x00
    };

    // get actual CRC and put low in ping[8] and high in ping[9]
    const uint16_t crc = update_crc(0, ping.data(), 8);
    ping[8] = static_cast<uint8_t>(crc & 0xFF);
    ping[9] = static_cast<uint8_t>((crc >> 8) & 0xFF);

    // Reserve 14 bytes for status packet
    std::array<uint8_t, 14> response{};

    while (true) {
        ping_attempts = ping_attempts + 1;

        response.fill(0);
        packet_valid = false;

        HAL_HalfDuplex_EnableTransmitter(&huart1);

        tx_result = HAL_UART_Transmit(
            &huart1,
            ping.data(),
            ping.size(),
            100 //HAL_MAX_DELAY
        );

        //Only receive if transmit successful
        if (tx_result == HAL_OK) {
            HAL_HalfDuplex_EnableReceiver(&huart1);

            rx_result = HAL_UART_Receive(
                &huart1,
                response.data(),
                response.size(),
                100
            );

            for (std::size_t i = 0; i < response.size(); ++i) {
                received_data[i] = response[i];
            }

            if (rx_result == HAL_OK) {
                // Get CRC sent by servo from last 2 bytes
                const uint16_t received_crc =
                    static_cast<uint16_t>(response[12]) |
                    (static_cast<uint16_t>(response[13]) << 8);

                // Calc CRC from received packet to compare
                const uint16_t calculated_crc =
                    update_crc(0, response.data(), 12);

                // Make sure response valid Ddynamixel status packet
                packet_valid =
                    response[0] == 0xFF &&
                    response[1] == 0xFF &&
                    response[2] == 0xFD &&
                    response[3] == 0x00 &&
                    response[4] == DXL_ID &&
                    response[5] == 0x07 &&
                    response[6] == 0x00 &&
                    response[7] == INST_STATUS &&
                    received_crc == calculated_crc;

                if (packet_valid) {
                    servo_error = response[8];

                    model_number =
                        static_cast<uint16_t>(response[9]) |
                        (static_cast<uint16_t>(response[10]) << 8);

                    firmware_version = response[11];

                    ping_responses = ping_responses + 1;
                }
            }
        }
        else {
            rx_result = HAL_ERROR;
        }

        HAL_Delay(1000);
    }
}