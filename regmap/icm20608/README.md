# ICM20608 — Regmap SPI IMU Driver

Demonstrates the Linux regmap API by driving an ICM20608 6-axis IMU
(gyroscope + accelerometer + temperature) over SPI, using
`module_spi_driver` + `misc_register`.

## What It Demonstrates

Regmap works identically across I2C and SPI. The only difference is
the initialization call:

```c
// I2C: devm_regmap_init_i2c(client, &config)
// SPI: devm_regmap_init_spi(spi, &config)

// After init, all register access is identical:
regmap_read(regmap, reg, &val);
regmap_write(regmap, reg, val);
```

For SPI, the `read_flag_mask = 0x80` tells regmap to set the MSB
for read operations — a common SPI protocol convention.

## Repository Structure

```
regmap/icm20608/
├── icm20608_regmap.c    # SPI regmap driver
├── icm20608App.c        # Test application
├── icm20608reg.h        # Register definitions
├── Makefile             # Build configuration
└── README.md            # This file
```
