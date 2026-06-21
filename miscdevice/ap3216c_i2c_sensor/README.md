# AP3216C I2C Sensor Driver

Demonstrates the Linux I2C subsystem by driving an AP3216C ambient light
and proximity sensor (ALS/PS) over the I2C bus.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Display** | 7-inch RGB LCD, 1024x600, 16bpp RGB565 |
| **Touch** | Goodix GT911 (I2C) |
| **Audio** | WM8960 codec (I2C) |
| **Sensor** | AP3216C (ALS + PS + IR), I2C address 0x1e |
| **Ethernet** | 10/100Mbps, APIPA (169.254.x.x) |
| **Serial** | CH340 USB-UART, 115200 baud |

## Development Environment

| Tool | Details |
|------|---------|
| **Host** | Windows 10 + WSL2 (Ubuntu 24.04) |
| **Cross-compiler** | Linaro GCC 5.3.1 |
| **Kernel source** | `linux-imx-4.1.15-2.1.0-g3dc0a4b-v2.7` (factory release) |
| **Build flags** | `ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-` |
| **Serial access** | usbipd-win (COM4 -> WSL2 `/dev/ttyUSB0`) |

## What It Demonstrates

```
Application reads /dev/ap3216c_demo
    |
    v
Driver reads AP3216C registers via I2C
    | i2c_transfer(adapter, msg, 2)
    v
AP3216C sensor returns IR, ALS, PS data
    |
    v
Data returned to userspace as 3 x short (6 bytes)
```

Key concepts:
- **I2C subsystem**: `struct i2c_client`, `struct i2c_msg`, `i2c_transfer()`
- **i2c_driver**: probe receives `i2c_client` instead of `platform_device`
- **miscdevice**: simplified char device registration (`misc_register`)
- **DTB matching**: I2C device declared as child of I2C controller node

## Repository Structure

```
miscdevice/
└── ap3216c_i2c_sensor/
    ├── ap3216c_demo.c       # I2C sensor driver source
    ├── ap3216cApp.c         # Test application
    ├── ap3216creg.h         # Sensor register definitions
    ├── Makefile             # Build configuration
    └── README.md            # This file
```

## Build

```bash
# Build the driver module and test app
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- app
```

## DTS Requirement

The AP3216C device must be declared as a child of the I2C1 controller node:

```dts
&i2c1 {
    ap3216c@1e {
        compatible = "ap3216c";
        reg = <0x1e>;
    };
};
```
