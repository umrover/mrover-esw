/* science_test */

#include <cstdlib>
#include <cstdint>
#include <bit>

#ifndef science_test_H
#define science_test_H

class VECTOR__INDEPENDENT_SIG_MSG {
    public:
    // Signals
    bool ISH_EnableWLED2;
    bool ISH_EnableWLED1;
    bool ISH_EnableHeater2;
    bool ISH_EnableHeater1;
    bool ISH_EnableHeater2AS;
    bool ISH_EnableHeater1AS;

    // Byte array representation of message
    uint8_t msg_arr[1];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    VECTOR__INDEPENDENT_SIG_MSG(uint8_t * byte_arr) {
    }

    // Encode into byte array format
    VECTOR__INDEPENDENT_SIG_MSG(
        bool ISH_EnableWLED2,
        bool ISH_EnableWLED1,
        bool ISH_EnableHeater2,
        bool ISH_EnableHeater1,
        bool ISH_EnableHeater2AS,
        bool ISH_EnableHeater1AS
    ) {
    }
};

class Science_Sensors {
    public:
    // Signals
    float Sensors_Temperature;
    float Sensors_Humidity;
    float Sensors_UV;
    float Sensors_Oxygen;

    // Byte array representation of message
    uint8_t msg_arr[16];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    Science_Sensors(uint8_t * byte_arr) {

        
        uint32_t temp_Sensors_Temperature = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            temp_Sensors_Temperature  |= byte_arr[i] << 8*i;
        }
        Sensors_Temperature = bit_cast<float>(temp_Sensors_Temperature);

        
        uint32_t temp_Sensors_Humidity = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            temp_Sensors_Humidity  |= byte_arr[i] << 8*i;
        }
        Sensors_Humidity = std::bit_cast<float>(temp_Sensors_Humidity);

        
        uint32_t temp_Sensors_UV = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            temp_Sensors_UV  |= byte_arr[i] << 8*i;
        }
        Sensors_UV = std::bit_cast<float>(temp_Sensors_UV);

        
        uint32_t temp_Sensors_Oxygen = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            temp_Sensors_Oxygen  |= byte_arr[i] << 8*i;
        }
        Sensors_Oxygen = std::bit_cast<float>(temp_Sensors_Oxygen);
    }

    // Encode into byte array format
    Science_Sensors(
        float Sensors_Temperature,
        float Sensors_Humidity,
        float Sensors_UV,
        float Sensors_Oxygen
    ) {
        for (uint8_t i = 0; i < ; ++i) {
            Sensors_Temperature[i] = 
        }
        for (uint8_t i = 0; i < ; ++i) {
            Sensors_Humidity[i] = 
        }
        for (uint8_t i = 0; i < ; ++i) {
            Sensors_UV[i] = 
        }
        for (uint8_t i = 0; i < ; ++i) {
            Sensors_Oxygen[i] = 
        }
    }
};

class Science_ISHOutbound {
    public:
    // Signals
    float ISH_Heater1Temp;
    float ISH_Heater2Temp;
    bool ISH_Heater1State;
    bool ISH_Heater2State;
    bool ISH_WLED1State;
    bool ISH_WLED2State;

    // Byte array representation of message
    uint8_t msg_arr[9];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    Science_ISHOutbound(uint8_t * byte_arr) {

        
        uint32_t temp_ISH_Heater1Temp = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            temp_ISH_Heater1Temp  |= byte_arr[i] << 8*i;
        }
        ISH_Heater1Temp = std::bit_cast<float>(temp_ISH_Heater1Temp);

        
        uint32_t temp_ISH_Heater2Temp = 0;
        for (uint8_t i = 0; i < 4; ++i) {
            temp_ISH_Heater2Temp  |= byte_arr[i] << 8*i;
        }
        ISH_Heater2Temp = std::bit_cast<float>(temp_ISH_Heater2Temp);
    }

    // Encode into byte array format
    Science_ISHOutbound(
        float ISH_Heater1Temp,
        float ISH_Heater2Temp,
        bool ISH_Heater1State,
        bool ISH_Heater2State,
        bool ISH_WLED1State,
        bool ISH_WLED2State
    ) {
        for (uint8_t i = 0; i < ; ++i) {
            ISH_Heater1Temp[i] = 
        }
        for (uint8_t i = 0; i < ; ++i) {
            ISH_Heater2Temp[i] = 
        }
    }
};

class Science_ISHInbound {
    public:
    // Signals
    bool ISH_Heater1Enable;
    bool ISH_Heater2Enable;
    bool ISH_Heater1EnableAS;
    bool ISH_Heater2EnableAS;
    bool ISH_WLED1Enable;
    bool ISH_WLED2Enable;

    // Byte array representation of message
    uint8_t msg_arr[1];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    Science_ISHInbound(uint8_t * byte_arr) {
    }

    // Encode into byte array format
    Science_ISHInbound(
        bool ISH_Heater1Enable,
        bool ISH_Heater2Enable,
        bool ISH_Heater1EnableAS,
        bool ISH_Heater2EnableAS,
        bool ISH_WLED1Enable,
        bool ISH_WLED2Enable
    ) {
    }
};


#endif /* science_test */