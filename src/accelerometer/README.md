# Accelerometer Project

## Hardware

### Accelerometer
 [Hardware Link](http://hiletgo.com/ProductDetail/2157948.html)  
 [MPU-6050 Product Spec](https://www.cdiweb.com/datasheets/invensense/mpu-6050_datasheet_v3%204.pdf)  
 [MPU-6050 Register Map](https://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/RM-MPU-6000A.pdf)  

### Pi5
 [WaveShare Pi Hats](https://www.waveshare.com/wiki/2-CH_CAN_FD_HAT#Stack_Mode)  
 [Pi I2C Pinout](https://pinout.xyz/pinout/i2c)  

## Notes
- Canalyzer Pi5 configuration has GPIO_0-GPIO_5 available for 3 I2C busses.  
    - i2c0 - GPIO_0 and GPIO_1
    - i2c1 - GPIO_2 and GPIO_3
    - i2c2 - GPIO_4 and GPIO_5
- MPU-6050 supports to addresses using LSB of addresses programmed through AD0 pin
- Use ```<linux/i2c-dev.h>``` for I2C interface on Pi


## References
 [Basic Guide to I2C (TI)](https://www.ti.com/lit/an/sbaa565/sbaa565.pdf?ts=1790482677674&ref_url=https%253A%252F%252Fwww.google.com%252F)  
 [I2C Dev Guide (NC State)](https://wordpress-courses2425.wolfware.ncsu.edu/ece-785-sprg-2025/wp-content/uploads/sites/80/2025/03/Linux-Device-Interfacing-I2C.pdf)