# LED MISC — Platform Driver + misc_register Example

Demonstrates the combination of `platform_driver` for DTB matching and
`misc_register` for simplified char device registration.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **LED** | GPIO1_IO03, active low |
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
platform_driver:
  DTB matches "atkalpha-gpioled" -> probe() called
    |
    v
  gpio_request() -> gpio_direction_output()
    |
    v
  misc_register() -> /dev/misc-led created (major=10, minor=dynamic)
    |
    v
  Application: open("/dev/misc-led") -> write(0/1) -> LED off/on
```

Key concepts:
- **misc_register** replaces: `alloc_chrdev_region` + `cdev_init` + `cdev_add` + `class_create` + `device_create`
- **Major=10** is reserved for all MISC devices, minor is dynamically assigned
- **platform_driver** provides automatic DTB matching via `of_match_table`

## Repository Structure

```
miscdevice/
└── led_misc_platform/
    ├── misc_platform.c    # platform_driver + misc_register
    └── README.md          # This file
```

## Build

```bash
# Build the driver
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

## DTS Requirement

The device tree must have a node with `compatible = "atkalpha-gpioled"`
containing a `led-gpio` property:

```dts
gpioled {
    compatible = "atkalpha-gpioled";
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_led>;
    led-gpio = <&gpio1 3 1>;
    status = "okay";
};
```
