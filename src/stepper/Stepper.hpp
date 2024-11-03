class Stepper {
public:
    // initialize with required pins DIR and STEP   
    Stepper(unsigned int DIR_pin, unsigned int STEP_pin) : 
    DIR_pin(DIR_pin), STEP_pin(STEP_pin) {};

    // set the optional pins
    void setENABLE_Pin(unsigned int ENABLE_pin);
    void setMS1_Pin(unsigned int MS1_pin);
    void setMS2_Pin(unsigned int MS2_pin);
    void setSLP_Pin(unsigned int SLP_pin);
    void setRST_Pin(unsigned int RST_pin);

    // change internal time delay (when flipping STEP)
    void setSpeed(unsigned int rpm); 

    // config pin logic
    void enable();  // ENABLE -> LOW
    void disable(); // ENABLE -> HIGH

    void setResolution(/*however we define resolution (4 options)*/); // adjust MS1 and MS2 resolution

    void sleep();   // SLP -> LOW
    void wake();    // SLP -> HIGH

    void reset();   // UHHHHH?

    // movement pin logic
    void setDirection(bool direction); // sets direction of stepper; true is clockwise, false is counterclockwise

    float readAngle(); // reads value of current angle?
    void writeAngle(float angle); // writes the angle of the stepper?


private:

    void setSTEP(int value);
    void setDIR(int value);
    void setRes(int value1, int value2);
    
    unsigned int DIR_pin;
    unsigned int STEP_pin;
    unsigned int ENABLE_pin;
    unsigned int MS1_pin;
    unsigned int MS2_pin;
    unsigned int SLP_pin;
    unsigned int RST_pin;
    int EN;
    int DIR; // true is clockwise, false is counterclockwise?
    int STEP;
    int MS1;
    int MS2;
    int SLP;
    int RST;

    float delay; // calculated from set speed (do we need a speed variable?)
    float step; // degrees
    float angle; // current angle?

    // check for max in one direction
    
};



/*
1) RST      Logic Input. When set LOW, all STEP commands are ignored and all FET functionality is turned off. Must be pulled HIGH to enable STEP control.
2) SLP      Logic Input. When pulled LOW, outputs are disabled and power consumption is minimized.

2) ENABLE   Logic Input. Enables the FET functionality within the motor driver. If set to HIGH, the FETs will be disabled, and the IC will not drive the motor. If set to LOW, all FETs will be enabled, allowing motor control.
        LOW     other functions do things
        HIGH    motor will be unaffected 

4) STEP     Logic Input. Any transition on this pin from LOW to HIGH will trigger the motor to step forward one step. Direction and size of step is controlled by DIR and MSx pin settings. This will either be 0-5V or 0-3.3V, based on the logic selection.


5) DIR      Logic Input. This pin determines the direction of motor rotation. Changes in state from HIGH to LOW or LOW to HIGH only take effect on the next rising edge of the STEP command. This will either be 0-5V or 0-3.3V, based on the logic selection.


6) MS1      Logic Input. See truth table below for HIGH/LOW functionality.
7) MS2      Logic Input. See truth table below for HIGH/LOW functionality.

Microstep Select Resolution Truth Table:      v     double check    v
MS1 	MS2 	Microstep Resolution        Step Angle      Step % of Revolution
L	    L	    Full Step (2 Phase)         1.8             1/200
H	    L	    Half Step                   0.9             1/400
L	    H	    Quarter Step                0.45            1/800
H	    H	    Eigth Step (Default)        0.225           1/1600


*/

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