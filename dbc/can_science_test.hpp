#include <cstdint>

struct VECTOR__INDEPENDENT_SIG_MSG {
	bool ISH_EnableWLED2;
	bool ISH_EnableWLED1;
	bool ISH_EnableHeater2;
	bool ISH_EnableHeater1;
	bool ISH_EnableHeater2AS;
	bool ISH_EnableHeater1AS;
};

struct Science_Sensors {
	float Sensors_Temperature;
	float Sensors_Humidity;
	float Sensors_UV;
	float Sensors_Oxygen;
};

struct Science_ISHOutbound {
	float ISH_Heater1Temp;
	float ISH_Heater2Temp;
	bool ISH_Heater1State;
	bool ISH_Heater2State;
	bool ISH_WLED1State;
	bool ISH_WLED2State;
};

struct Science_ISHInbound {
	bool ISH_Heater1Enable;
	bool ISH_Heater2Enable;
	bool ISH_Heater1EnableAS;
	bool ISH_Heater2EnableAS;
	bool ISH_WLED1Enable;
	bool ISH_WLED2Enable;
};

