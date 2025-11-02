#include <iostream>
#include <bitset>
#include "science_test.h"

int main() {
    Science_ISHOutbound input(4.333, -0.123, true, false, false, true);
    std::bitset<8> binary(input.msg_arr[8]);
    std::cout << input.ISH_Heater1Temp << '\n'
                << input.ISH_Heater2Temp << '\n'
                << input.ISH_Heater1State << '\n'
                << input.ISH_Heater2State << '\n'
                << input.ISH_WLED1State << '\n'
                << input.ISH_WLED2State << '\n'
                << binary << '\n' << '\n';
    Science_ISHOutbound output(input.msg_arr);
    std::cout << output.ISH_Heater1Temp << '\n'
                << output.ISH_Heater2Temp << '\n'
                << output.ISH_Heater1State << '\n'
                << output.ISH_Heater2State << '\n'
                << output.ISH_WLED1State << '\n'
                << output.ISH_WLED2State << '\n';
}