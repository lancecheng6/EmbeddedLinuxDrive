# ICM20608 SPI IMU Driver

Demonstrates the Linux SPI subsystem by driving an ICM20608 6-axis IMU
(gyroscope + accelerometer + temperature) over the SPI bus.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Sensor** | ICM20608 (gyro + accel + temp), SPI, CS0 |
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
SPI driver matches DTB node "alientek,icm20608"
    |
    v
icm20608_probe(spi_device)
    | spi->mode = SPI_MODE_0
    | spi_setup(spi)
    | misc_register() -> /dev/icm20608
    | icm20608_reginit() -> configure sensor registers
    v
Application reads 7 x signed int (gyro3 + accel3 + temp)
    |
    v
Driver reads 14 bytes via SPI from sensor registers
    | spi_sync(spi, &m)  <-- spi_message + spi_transfer
    v
Raw values converted to physical units in userspace
```

Key concepts:
- **SPI subsystem**: `struct spi_device`, `struct spi_message`, `struct spi_transfer`
- **spi_driver**: probe receives `spi_device` (similar to i2c_client)
- **spi_sync**: synchronous SPI transfer (TX = register address, RX = data)
- **module_spi_driver**: convenience macro replacing module_init/module_exit

## Repository Structure

```
spi/
└── icm20608/
    ├── icm20608.c        # SPI IMU driver
    ├── icm20608App.c     # Test application
    ├── icm20608reg.h     # Sensor register definitions
    ├── Makefile          # Build configuration
    └── README.md         # This file
```

## Build

```bash
# Build the driver module
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

# Build the test application
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- app
```

## Deploy & Test

```bash
# Copy to target board
scp icm20608.ko icm20608App root@<BOARD_IP>:/home/root/

# Load the driver
insmod /home/root/icm20608.ko
# Kernel: ICM20608 ID = 0xAE  (or 0xAF)

# Read sensor data
./icm20608App /dev/icm20608

# Unload
rmmod icm20608
```

## DTS Requirement

The ICM20608 device must be declared as a child of the SPI3 controller:

```dts
&ecspi3 {
    icm20608@0 {
        compatible = "alientek,icm20608";
        spi-max-frequency = <8000000>;
        reg = <0>;
    };
};
```
