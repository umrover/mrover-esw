/* science_test */

#include <cstdlib>

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
    uint8_t msg_arr[6];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    VECTOR__INDEPENDENT_SIG_MSG(uint8_t * byte_arr) {
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
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
}

class Science_Sensors {
    public:
    // Signals
    float Sensors_Temperature;
    float Sensors_Humidity;
    float Sensors_UV;
    float Sensors_Oxygen;

    // Byte array representation of message
    uint8_t msg_arr[128];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    Science_Sensors(uint8_t * byte_arr) {
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
    }

    // Encode into byte array format
    Science_Sensors(
        float Sensors_Temperature,
        float Sensors_Humidity,
        float Sensors_UV,
        float Sensors_Oxygen
    ) {

    }
}

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
    uint8_t msg_arr[68];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    Science_ISHOutbound(uint8_t * byte_arr) {
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
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

    }
}

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
    uint8_t msg_arr[6];

    // Constructors serve as our encode/decode functions
    
    // Decode into message class format
    Science_ISHInbound(uint8_t * byte_arr) {
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
        
        // TODO: oh lawd
        

        
        
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
}


#endif /* science_test */