# Watchdog — Hardware Timer Test Program

Demonstrates the Linux watchdog (hardware timer) framework on the
I.MX6ULL platform. The watchdog is a hardware counter that resets
the entire SoC if not periodically "fed" (kicked) by software.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Display** | 7-inch RGB LCD, 1024x600, 16bpp RGB565 |
| **Touch** | Goodix GT911 (I2C) |
| **Audio** | WM8960 codec (I2C) |
| **Watchdog** | Dual wdt controllers, `CONFIG_IMX2_WDT=y` |
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
Userspace program (watchdog_test)
    |
    open("/dev/watchdog")
    ioctl(WDIOC_SETTIMEOUT, 10s)
    ioctl(WDIOC_ENABLECARD)
    |
    while (1) {
        ioctl(WDIOC_KEEPALIVE)   ← feed the dog
        sleep(9.9)
    }
    |
    [if program stops → watchdog expires → SoC hard reset]
```

The watchdog driver is built into the kernel (`CONFIG_IMX2_WDT=y`).
No kernel driver needs to be written. The test program controls the
watchdog entirely through standard Linux ioctl calls.

Key concepts:
- **Watchdog**: hardware timer that auto-resets the SoC on expiry
- **Feeding**: periodic `WDIOC_KEEPALIVE` ioctl resets the counter
- **System recovery**: if the application hangs, watchdog fires → reboot
- **Single-access**: `/dev/watchdog` can only be opened by one process

## Repository Structure

```
watchdog/test/
├── watchdog_test.c   # Watchdog test program
└── README.md         # This file
```

## Build

Cross-compile for ARM:

```bash
arm-linux-gnueabihf-gcc -o watchdog_test watchdog_test.c
```

## Deploy & Run

```bash
# Upload to board
scp watchdog_test root@<BOARD_IP>:/home/root/

# Set a 10-second timeout and feed the dog
./watchdog_test 10

# To test watchdog reboot: let the program be killed
# (e.g. timeout 5 ./watchdog_test 10)
# After the timeout, the watchdog expires → board auto-reboots
```

## Test Procedure

```bash
# Terminal 1: run the watchdog program with 8-second timeout
timeout 6 ./watchdog_test 8

# After 6 seconds:
#   - timeout kills the program
#   - watchdog timer continues counting
#   - ~2 seconds later → watchdog expires
#   - SoC hard-resets → board reboots automatically
```