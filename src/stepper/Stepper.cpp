#include "Stepper.hpp"

void Stepper::reset()
{
  setSTEP(0);
  setDIR(0);
  setResolution(); // 1/8 microstep or 0.225 deg
  enable();
  step = 0.225;
}

void Stepper::enable()
{
    EN = 0; // active low
}

void Stepper::disable()
{
    EN = 1;
}

void Stepper::setRes(int MS1_val, int MS2_val)
{
    MS1 = MS1_val;
    MS2 = MS2_val;

    step = MS1 ? (MS2 ? 0.225 : 0.9) : (MS2 ? 0.45 : 1.8);
}

void Stepper::setSTEP(int value)
{
    STEP = value;
}

void Stepper::setDIR(int value)
{
    DIR = value;
}

void Stepper::setAngle(int angleIN)
{
    int temp = angleIN / step;

    for (int i = 0; i < temp; i++)
    {
        digitalWrite(STEP_pin,1);
        delay(speed);
        digitalWrite(STEP_pin,0);
        angle += step;
    }
}

void Stepper::resetAngle()
{
    DIR = DIR ? 0 : 1;

    while (angle != 0)
    {
        setAngle(angle);
    }
}