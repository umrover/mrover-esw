#define steps = 200 // 360 deg/ 1.8 deg

class Stepper
{
public:
    Stepper()
    {

    } // base constructor

    Stepper(unsigned int pin1, unsigned int pin2, 
    unsigned int pin3, unsigned int pin4)
    {
        coil1_pin1 = pin1;
        coil1_pin2 = pin2;
        coil2_pin1 = pin3;
        coil2_pin2 = pin4;
        

    } // base constructor
    void setSpeed(unsigned int rpm);
    void attach(int pin); // Attatches the stepper to a pin
    void write(float angle); // writes the angle of the stepper
    float read(); // reads value of curretn angle
    void reset(); // sets stepper angle to 0
    void setDirection(bool direction); // sets direction of stepper; true is clockwise, false is counterclockwise
    void toggleOnOff(); // turns the stepper on or off

private:
    float delay;
    bool status;    // true is on, false is off
    bool direction; // true is clockwise, false is counterclockwise
    unsigned int pin;
    int max; // max pulse width in us that corresponds to max angle
    int min; // min pulse width in us that corresponds to min angle
    int speed; // rpm
    float angle; // 1.8 deg step
    float step = 1.8; // degrees
    unsigned int coil1_pin1;
    unsigned int coil1_pin2;
    unsigned int coil2_pin1;
    unsigned int coil2_pin2;
};

// Potential functions:
// On/Off: Will set the motor to “turn on” and run
// Set: will turn input angle degrees from current position
// Reset: will turn to a default position (0 deg)
// Select: With a given number of pots and which pot to direct to, turn to that pot
// Change motor speed
// Enable/Disable
// Forward/Reverse
// Microsteps
// Auto detect limits to prevent breakage
// If it has limits and is not 360
// Get functions for the internal variables
// More specific/complex functions as needed for the motor’s purposes
//
/* Functions from servo library
attach();
*Attach the Servo variable to a pin. 
Note that in Arduino IDE 0016 and earlier, 
the Servo library supports servos on only two pins: 
9 and 10
write();
writeMicroseconds();
read();
*Read the current value of the angle
attached();
detached();
*/