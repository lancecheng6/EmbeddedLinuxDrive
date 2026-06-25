# AP3216C — IIO I2C Sensor Driver

Demonstrates the Linux IIO (Industrial I/O) subsystem for sensor
drivers. No custom file_operations or /dev/ nodes — IIO provides
a standardized sysfs interface.

## What IIO Provides

```bash
# Standard sysfs interface (no application needed)
cat /sys/bus/iio/devices/iio:device0/in_intensity_both_raw
cat /sys/bus/iio/devices/iio:device0/in_proximity_raw
cat /sys/bus/iio/devices/iio:device0/in_intensity_ir_raw
```

## Repository Structure

```
iio/ap3216c/
├── ap3216c.c        # IIO driver (I2C + regmap)
├── ap3216creg.h     # Register definitions
├── Makefile
└── README.md
```

## Build & Test

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

# Deploy
scp ap3216c.ko root@<BOARD_IP>:/home/root/

# Load (unbind built-in first)
echo 0-001e > /sys/bus/i2c/drivers/ap3216c/unbind 2>/dev/null
insmod ap3216c.ko

# Read data
cat /sys/bus/iio/devices/iio:device0/in_intensity_both_raw

# Unload
rmmod ap3216c
```
