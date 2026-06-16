# Embedded Linux Drive

Embedded Linux driver development notes on I.MX6ULL.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Display** | 7-inch RGB LCD, 1024x600, 16bpp RGB565 |
| **Touch** | Goodix GT911 (I2C) |
| **Audio** | WM8960 codec (I2C) |
| **WiFi** | RTL8188EU USB dongle |
| **Ethernet** | 10/100Mbps, APIPA (169.254.x.x) |
| **Serial** | CH340 USB-UART, 115200 baud |

## Development Environment

| Tool | Details |
|------|---------|
| **Host** | Windows 10 + WSL2 (Ubuntu 24.04) |
| **Cross-compiler** | Linaro GCC 5.3.1 |
| **Kernel source** | `linux-imx-4.1.15-2.1.0-g3dc0a4b-v2.7` (factory release) |
| **Build flags** | `ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-` |
| **Serial access** | usbipd-win (COM4 → WSL2 `/dev/ttyUSB0`) |

## Repository Structure

```
EmbeddedLinuxDrive/
└── platform_driver/
    └── Toggle_GPIO/
        ├── gpioled_platform.c    # Platform LED driver
        ├── ledApp.c              # Test application
        └── final.dts             # Device Tree Source (modified)
```

## Build & Deploy

```bash
# Build driver
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

# Deploy to board
scp gpioled_platform.ko ledApp root@<BOARD_IP>:/home/root/

# Load and test
insmod gpioled_platform.ko
./ledApp /dev/gpioled 1   # LED ON
./ledApp /dev/gpioled 0   # LED OFF
```
