# Timers

The following information is for STM32 timers.
You will find similar concepts with different microcontrollers; however,
the terminology may differ.

## Configuring a Timer

There are three main parts that can control the behavior of a timer:

1. Counter Period (also Auto-Reload Register, or ARR)
2. Prescaler (PSC)
3. Count & Compare Register (CCR)

## Use Cases

### Periodic Timer

### Input-Capture

### Output-Capture

### PWM
PWM (pulse width modulation) is a digital signal that is set to high and low for a set amount of time.
The signal 

Devices that are
controlled by PWM will define a "period" that the signal should be set at.
use the fraction that the signal is high for to set something (eg. velocity, position, brightness).

There are 2 parameters that control what type of pulses the output pin can produce, the prescaler (PSC) and the auto-reload register (ARR):
* PSC: changes the frequency of the clock by dividing it, which adjusts how fast or slow the cycles are. For example, if a clock has a frequency of 8MHz, a PSC of 7 would make the frequency 1 MHz ( Clk/(PSC+1) ).
* ARR: defines the number of ticks in one cycle, which adjusts how fine the PWM signal can be. For example, if the same clock as the previous example, with the same PSC (so 1MHz) is used with an ARR of 49, then each tick would be 20ns long ( 1ms/(ARR+1) ) and the width of each pulse can increment by 20ns.

If you are still confused, here's a helpful [slide deck](https://docs.google.com/presentation/d/1eK4ROr9wMi3IOqEUcBABVkFSWsVM56jEgm_zoR2-wio/edit?usp=drive_link) that goes more in depth. 


Read the [datasheet](http://www.ee.ic.ac.uk/pcheung/teaching/DE1_EE/stores/sg90_datasheet.pdf) for the servo and determine what PSC and ARR should be given that the clock frequency of the Nucleo is 72MHz.


Once you have determined and edited the timer config, save the file and generate code. Note: you can always come back to the .ioc to make changes.


