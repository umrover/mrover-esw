# Timers

The following information is for STM32 timers. You will find similar concepts with different
microcontrollers; however, the terminology and specifics may differ.

## Configuring a Timer

There are three main parts that can control the behavior of a timer:

1. Prescaler (PSC)
2. Counter Period (also Auto-Reload Register, or ARR)
3. Count & Compare Register (CCR)

### Prescaler (PSC)

This value determines the frequency of the timer. The timer frequency is calculated by the
following equation: `Clock Frequency / (PSC+1)`.

For example, if our clock frequency is 72 MHz and we set PSC = 71, our timer will tick at a
frequency of 1 MHz.

### Counter Period (Auto-Reload Register, or ARR)

This defines the number of ticks in one period. The period is calculated by the following equation:
`(1 / Timer Frequency) * (ARR + 1)`.

For example, if our timer frequency is 1 MHz, then the timer will tick every 1000 ns. So, if we
set the ARR = 4999, our period will be 0.005 seconds (`1000 ns * (4999 + 1) = 0.005 s`).

### Count & Compare Register (CCR)

This register holds a specific value that the timer tick counter is compared against. When the
counter reaches the value in the CCR, an event is triggered, such as an interrupt or output signal
change, which is commonly used for tasks like PWM generation or input capture.

## Use Cases

### Periodic Timer

The simplest use: run the counter and raise an interrupt every time it reaches ARR. Set PSC and
ARR for the period you want, start the timer in interrupt mode, and do the work in the callback.

This is how the boards drive fixed-rate behavior such as polling a sensor or publishing a CAN
message at a set frequency. Because the interrupt fires from hardware, the rate does not drift
with whatever else the main loop is doing.

Keep the callback short. It runs in interrupt context, so a slow callback delays every other
interrupt on the MCU, and one that outlasts the period means the next tick is missed entirely.

### Input Capture

Input capture records the counter value at the moment an edge arrives on a pin, in hardware. The
captured value lands in the CCR and an interrupt fires.

Because the timestamp is taken by the timer rather than by software, it does not suffer from
interrupt latency. This makes it the right tool for measuring things about an incoming signal:

- **Frequency**: capture on successive rising edges; the difference in counts is the period.
- **Duty cycle**: capture on both edges and compare the high time against the period.
- **Event timing**: any case where you need to know *when* something happened, not just that it
  did.

Watch for counter overflow. If the counter rolls over between two captures, the naive difference
is wrong, so either count overflows in the update interrupt or pick PSC and ARR so the interval of
interest cannot wrap.

### Output Capture

Output compare is the inverse: when the counter reaches the CCR value, the timer changes an output
pin in hardware, with no software involved in the transition. The pin can be set, cleared, or
toggled.

Toggling on compare gives a square wave whose frequency depends only on the timer configuration,
which is useful for generating a clock or a tone. PWM, below, is output compare with the compare
value used to control the width of the pulse rather than to toggle at a fixed point.

The advantage over setting a pin from a callback is timing accuracy: the edge happens exactly at
the compare match, not whenever the interrupt gets serviced.

### PWM

#### What Is PWM?
PWM (pulse width modulation) is a digital signal that is set to high and low for a set amount of
time to represent a percentage.

#### Why Do We Use PWM?
Over a wire, we can only send a value of 0 (low voltage) or 1 (high voltage). However, let's say
that I want to control the percent brightness of an LED. How can I send this percentage (a value
between 0 and 1) to the LED with just a wire that can only send 0 or 1? This is where PWM comes
in. To send a value of 20% (0.20) using PWM, we first set a specified **period**. Then, for the
first 20% of the period, we would set the wire to 1. For the rest of the period, we set the wire to
0. This gives us a **duty cycle** of 20%.

#### Configuring an STM32 PWM Timer
For PWM, we will have to modify the PSC, ARR, and CCR registers. The PSC and ARR registers can
be modified in the `.ioc` in order to set a constant period for the PWM signal. Then, in our code
we can modify the CCR register in order to set different PWM signals. For normal PWM generation,
the PWM signal will be set high in the beginning of the period. Then, when the timer counter goes
above the value of CCR, the PWM signal will be set low.

If you are still confused, here's a helpful [slide deck](https://docs.google.com/presentation/d/1eK4ROr9wMi3IOqEUcBABVkFSWsVM56jEgm_zoR2-wio/edit?usp=drive_link)
that goes more in depth. 
