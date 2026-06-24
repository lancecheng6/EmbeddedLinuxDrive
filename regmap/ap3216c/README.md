# AP3216C — Regmap I2C Sensor Driver

Demonstrates the Linux regmap API by driving an AP3216C ambient light
+ proximity + IR sensor over I2C, using `module_i2c_driver` + `misc_register`.

## What It Demonstrates

Regmap provides a unified register access API that works across I2C, SPI,
and MMIO buses. Compare with the raw I2C version in `miscdevice/ap3216c_i2c_sensor/`.

```c
// Raw I2C: manual i2c_msg + i2c_transfer
struct i2c_msg msg[2];
msg[0].buf = &reg;
msg[1].flags = I2C_M_RD;
i2c_transfer(client->adapter, msg, 2);

// Regmap: one line
regmap_read(regmap, reg, &val);
```

## Repository Structure

```
regmap/ap3216c/
├── ap3216c_regmap.c    # I2C regmap driver
├── ap3216cApp.c        # Test application
├── ap3216creg.h        # Register definitions
├── Makefile            # Build configuration
└── README.md           # This file
```
