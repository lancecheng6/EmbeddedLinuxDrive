# Timer -> Tasklet -> GPIO Toggling Demo

Demonstrates the Linux kernel's top-half / bottom-half interrupt model
using a kernel timer as the interrupt source and a tasklet as the
bottom-half handler.

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
| **Serial access** | usbipd-win (COM4 -> WSL2 `/dev/ttyUSB0`) |

## What It Demonstrates

```
Kernel timer fires every 10 seconds (simulates an interrupt)
    |
    v
timer_callback()              <-- top half (softirq context)
    | tasklet_schedule()
    v
led_tasklet_function()        <-- bottom half (tasklet context)
    | gpio_set_value() toggles LED
    v
LED on/off toggles every 10 seconds
```

Key concepts:
- **Top half**: runs in softirq context, must not sleep, does minimal work
- **Bottom half**: runs in tasklet context (software interrupt), does the real work
- **No userspace app required**: the driver operates autonomously

## Repository Structure

```
platform_driver/
└── Timer_Tasklet_Demo/
    ├── timer_tasklet_platform.c    # Platform driver source
    ├── Makefile                    # Build configuration
    └── README.md                   # This file
```

## Build

```bash
# Build the driver module
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

## Deploy & Test

```bash
# Copy to target board
scp timer_tasklet_platform.ko root@<BOARD_IP>:/home/root/

# Load the driver — LED starts toggling every 10 seconds automatically
insmod /home/root/timer_tasklet_platform.ko

# Optional: manual control via /dev/tasklet-led
# echo -n -e '\\x01' > /dev/tasklet-led   # LED ON (stops timer)
# echo -n -e '\\x00' > /dev/tasklet-led   # LED OFF (stops timer)

# Unload
rmmod timer_tasklet_platform
```