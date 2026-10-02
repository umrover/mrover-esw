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
- MPU-6050 supports two addresses using LSB of addresses programmed through AD0 pin (AD0 low = 0x68, high = 0x69)
- Use ```<linux/i2c-dev.h>``` for I2C interface on Pi


## Software

Polls one MPU-6050 (AD0 low, `0x68`) on each I2C bus listed in `accelerometer_config.yaml`
and batches the readings into InfluxDB (`accelerometer_db`). One poller thread runs per bus,
and a single committer thread POSTs everything that was queued every `commit_period_ms`
(2 s timeout; a failed POST is logged and that batch is dropped).

**This stack piggybacks on canalyzer.** InfluxDB and Grafana are run by the canalyzer stack
(`src/canalyzer/compose.yaml`); this compose file only runs the driver container, which writes
to canalyzer's InfluxDB on host port 8086. On start, `scripts/init_influx.sh` waits for the
`influxdb` container to be healthy, then creates `accelerometer_db` and grants `logger` write
and `grafana` read access (idempotent). In Grafana, add a second InfluxDB datasource pointing at
`accelerometer_db`.

```
Core/Inc, Core/Src
  i2c_bus      - /dev/i2c-N wrapper using I2C_RDWR (linux/i2c-dev.h)
  mpu6050      - init + 14 byte burst read of accel/gyro (±2 g, ±250 dps, ~44 Hz DLPF)
  committer    - InfluxDB line protocol builder + batching commit thread
  accelerometer.cpp - reads config/env, starts pollers and committer
```

### InfluxDB format
Two measurements, both tagged with the I2C bus name, timestamps in ms:
```
accelerometer,bus_name=i2c-1 x=0.01221,y=-0.00342,z=0.99878 <ts>   # g
gyroscope,bus_name=i2c-1 x=-0.45802,y=0.21374,z=0.07634 <ts>        # deg/s
```
```
docker exec -it influxdb influx -username admin -password password
USE accelerometer_db;
SELECT * FROM accelerometer WHERE bus_name = 'i2c-1' LIMIT 10;
```

### Scripts
- `./scripts/dockerBuild.sh` - builds `accelerometer_image`
- `./scripts/start.sh` - runs the driver in the foreground, ctrl+c to stop (canalyzer must be running)
- `./scripts/pi_start.sh` / `./scripts/pi_stop.sh` - detached start/stop, used by systemd
- `./scripts/init_influx.sh` - waits for canalyzer's InfluxDB and creates `accelerometer_db`
- `./scripts/accelerometer.service` - systemd unit, install instructions are in the file header.
  `Requires=`/`After=` canalyzer.service, and `PartOf=` it so stopping or restarting canalyzer
  does the same to the accelerometer

A missing `/dev/i2c-N` is fatal at startup (container restarts). A sensor that stops
responding is re-initialized once a second without restarting.

## Startup
- make sure the canalyzer stack is up (`src/canalyzer/scripts/start.sh` or `canalyzer.service`)
- run `./scripts/dockerBuild.sh` if you haven't built (or after changing code)
- run `./scripts/start.sh`
- crtl+c to end, let script end gracefully

```
docker logs -f accelerometer_instance   # driver output (init/read errors, influx commit errors)
systemctl status accelerometer          # if running through systemd
journalctl -u accelerometer             # systemd start/stop logs
```

### Running without docker
Useful for checking a sensor on the Pi before building the image. Needs cmake and a C++23 compiler.
```
cmake -S src/accelerometer -B build/accelerometer
cmake --build build/accelerometer --target accelerometer -j
INFLUXDB_HOST=127.0.0.1 INFLUXDB_PORT=8086 INFLUXDB_DB=accelerometer_db \
INFLUXDB_USER=logger INFLUXDB_PASSWORD=password \
./build/accelerometer/accelerometer src/accelerometer/accelerometer_config.yaml
```
Your user needs to be in the `i2c` group to open `/dev/i2c-*` (`sudo usermod -aG i2c $USER`, then log back in).

### Grafana
Add an InfluxDB datasource (InfluxQL) with URL `http://influxdb:8086`, database `accelerometer_db`,
user `grafana` / `password`. Example panel query:
```
SELECT mean("x"), mean("y"), mean("z") FROM "accelerometer" WHERE "bus_name" = 'i2c-0' AND $timeFilter GROUP BY time($__interval)
```

## Wiring
One HiLetgo GY-521 (MPU-6050) per bus. Pin numbers are physical header pins, see [pinout.xyz](https://pinout.xyz/pinout/i2c).

| Bus (`/dev/`) | SDA            | SCL            |
|---------------|----------------|----------------|
| `i2c-0`       | GPIO0, pin 27  | GPIO1, pin 28  |
| `i2c-1`       | GPIO2, pin 3   | GPIO3, pin 5   |
| `i2c-2`       | GPIO4, pin 7   | GPIO5, pin 29  |

| GY-521 | Pi                                                           |
|--------|--------------------------------------------------------------|
| VCC    | 3V3 (pin 1 or 17)                                            |
| GND    | any GND (e.g. pin 6, 9, 14)                                  |
| SCL    | bus SCL from table above                                     |
| SDA    | bus SDA from table above                                     |
| AD0    | GND, address 0x68 (the code expects AD0 low on every bus)    |
| XDA, XCL, INT | not connected                                         |

- GPIO2/3 have pull-ups on the Pi board. GPIO0/1 and GPIO4/5 rely on the GY-521's on-board
  pull-ups and the weak internal pull-ups the overlays enable. Keep wires short; on long runs drop the baudrate.
- GPIO0/1 are the HAT ID EEPROM pins (ID_SD/ID_SC). The Pi 5 is fine using them as a normal bus
  once booted; a HAT EEPROM, if present, shows up at 0x50 on `i2c-0` and doesn't clash with 0x68.
- The CAN FD HAT uses SPI0/SPI1 and interrupt GPIOs 25, 13, 24, 23 (see canalyzer README), none of which overlap GPIO0-5.

## Setting up Pi 5 I2C (i2c0 - i2c2)
Pi 5 I2C buses live on the RP1 chip and are enabled with the `-pi5` overlays
([overlay README](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/arch/arm/boot/dts/overlays/README)).
Their default pins are exactly the primary pins above, so no pin params are needed.

- step 1, enable i2c1 and the `i2c-dev` kernel module (creates `/dev/i2c-*`)
```
sudo raspi-config
Choose Interface Options -> I2C -> Yes
```
- step 2, enable i2c0 and i2c2
```
sudo nano /boot/firmware/config.txt   # /boot/config.txt on older images
# add at the end of config.txt (next to the canalyzer mcp251xfd lines)
dtparam=i2c_arm=on       # i2c1 on GPIO2/3 (raspi-config already adds this)
dtoverlay=i2c0-pi5       # i2c0 on GPIO0/1 (default pins_0_1)
dtoverlay=i2c2-pi5       # i2c2 on GPIO4/5 (default pins_4_5)
# optional: run a bus at 400 kHz (MPU-6050 max), e.g.
# dtoverlay=i2c2-pi5,baudrate=400000
# dtparam=i2c_arm_baudrate=400000
```
- step 3, if you skipped raspi-config, load `i2c-dev` on boot yourself
```
echo i2c-dev | sudo tee -a /etc/modules
```
- step 4, reboot and check
```
sudo reboot
sudo apt-get install i2c-tools
ls /dev/i2c-*          # expect i2c-0, i2c-1, i2c-2 (plus Pi 5 internal buses like i2c-11, i2c-12, ignore those)
i2cdetect -l           # lists adapters, confirm i2c-0/1/2 are the RP1 buses
```
- step 5, check each sensor responds
```
i2cdetect -y 0         # should show 68 in the grid, repeat with 1 and 2
i2cget -y 0 0x68 0x75  # WHO_AM_I, should print 0x68
```

Example `i2cdetect -y 1` with a sensor attached:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- --
...
60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --
```

If a bus shows up under a different `/dev/i2c-N`, or a sensor is missing from `i2cdetect`, update
`buses:` in `accelerometer_config.yaml` and `devices:` in `compose.yaml` to match.

### setting up pi from raw
Same as canalyzer (git, docker, can-utils), plus:
```
sudo apt-get install i2c-tools
```
Then install the systemd unit (after `canalyzer.service`):
```
sudo cp src/accelerometer/scripts/accelerometer.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now accelerometer.service
```

## TODO
- calibrate per-sensor accel/gyro offsets
- make accel/gyro full scale range configurable from the yaml
- Grafana dashboard for the three buses

## References
 [Basic Guide to I2C (TI)](https://www.ti.com/lit/an/sbaa565/sbaa565.pdf?ts=1790482677674&ref_url=https%253A%252F%252Fwww.google.com%252F)  
 [I2C Dev Guide (NC State)](https://wordpress-courses2425.wolfware.ncsu.edu/ece-785-sprg-2025/wp-content/uploads/sites/80/2025/03/Linux-Device-Interfacing-I2C.pdf)