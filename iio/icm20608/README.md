# ICM20608 — IIO SPI IMU Driver

Demonstrates the Linux IIO (Industrial I/O) subsystem for IMU
sensors. No custom file_operations or /dev/ nodes — IIO provides
a standardized sysfs interface.

## What IIO Provides

```bash
# Standard sysfs interface (no application needed)
cat /sys/bus/iio/devices/iio:device0/in_anglvel_x_raw
cat /sys/bus/iio/devices/iio:device0/in_accel_x_raw
cat /sys/bus/iio/devices/iio:device0/in_temp_raw
```

## Repository Structure

```
iio/icm20608/
├── icm20608.c       # IIO driver (SPI + regmap)
├── icm20608reg.h    # Register definitions
├── Makefile
└── README.md
```
