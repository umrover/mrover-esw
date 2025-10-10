/* science_test */

#include <cstdlib>

#ifndef science_test_H
#define science_test_H

typedef struct {
    bool ISH_EnableWLED2;
    bool ISH_EnableWLED1;
    bool ISH_EnableHeater2;
    bool ISH_EnableHeater1;
    bool ISH_EnableHeater2AS;
    bool ISH_EnableHeater1AS;
} VECTOR__INDEPENDENT_SIG_MSG;

typedef struct {
    float Sensors_Temperature;
    float Sensors_Humidity;
    float Sensors_UV;
    float Sensors_Oxygen;
} Science_Sensors;

typedef struct {
    float ISH_Heater1Temp;
    float ISH_Heater2Temp;
    bool ISH_Heater1State;
    bool ISH_Heater2State;
    bool ISH_WLED1State;
    bool ISH_WLED2State;
} Science_ISHOutbound;

typedef struct {
    bool ISH_Heater1Enable;
    bool ISH_Heater2Enable;
    bool ISH_Heater1EnableAS;
    bool ISH_Heater2EnableAS;
    bool ISH_WLED1Enable;
    bool ISH_WLED2Enable;
} Science_ISHInbound;


#endif /* science_test */