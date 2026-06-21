# Key Input — Linux Input Subsystem Driver

Demonstrates the Linux input subsystem by registering a GPIO key as an
input device. The driver uses interrupt + timer debounce and reports
key events through the standard input core, creating `/dev/input/eventX`.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Display** | 7-inch RGB LCD, 1024x600, 16bpp RGB565 |
| **Key** | KEY0 on GPIO1_IO18, active low |
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
User presses KEY0 on the board
    |
    v
GPIO1_IO18 interrupt fires
    |
    v
key_irq_handler()              <-- top half (interrupt context)
    | mod_timer(&timer, 10ms)
    v
key_timer_function()           <-- bottom half (timer callback)
    | gpio_get_value() == 0    <-- debounce check
    | input_report_key(inputdev, KEY_0, 1)
    | input_sync(inputdev)
    v
Input core places event in queue
    |
    v
Userspace application reads /dev/input/eventX
    | struct input_event ev
    | ev.type == EV_KEY
    | ev.code == KEY_0 (11)
    | ev.value == 1 (pressed) / 0 (released)
```

Key concepts:
- **Input subsystem**: standardized API for all input devices
- **No custom char device**: `/dev/input/eventX` is created by the core
- **No file_operations needed**: driver only calls `input_report_key()` + `input_sync()`
- **platform_driver**: DTB matching via `of_match_table` + `module_platform_driver`

## Repository Structure

```
InputSubsystem/
└── key_input/
    ├── key_input_platform.c    # Input subsystem key driver
    ├── evtest.c                # Simple event test application
    ├── Makefile                # Build configuration
    └── README.md               # This file
```

## Build

```bash
# Build the driver module
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

## DTS Requirement

The device tree must have a key node with `compatible = "atkalpha-key"`
and GPIO interrupt configuration:

```dts
key {
    compatible = "atkalpha-key";
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_key>;
    key-gpio = <&gpio1 18 1>;
    interrupt-parent = <&gpio1>;
    interrupts = <18 3>;
    status = "okay";
};
```
