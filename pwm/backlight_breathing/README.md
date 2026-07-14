# LCD Backlight Breathing Effect

Demonstrates PWM backlight control on an I.MX6ULL board by creating a 
smooth breathing effect on the LCD backlight — brightness fades in and 
out gradually.


## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Display** | 7-inch RGB LCD, 1024x600, 16bpp RGB565 |
| **Touch** | Goodix GT911 (I2C) |
| **Audio** | WM8960 codec (I2C) |
| **Backlight** | PWM1 (pwm@02080000), 5ms period, 8 brightness levels |
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

This is a userspace program that controls hardware PWM through the Linux
sysfs interface. No kernel driver is needed — the PWM controller driver
is already built into the kernel (`CONFIG_PWM_IMX=y`).

Because the board's hardware PWM output pins are not connected to a
suitable LED, this experiment uses the LCD backlight to demonstrate the
breathing effect. The same principle applies to any PWM-controlled device.

```
backlight_breathing (userspace)
    |
    write() → /sys/class/backlight/backlight/brightness
    |
pwm-backlight driver (kernel built-in)
    |
    pwm_config(pwm1, duty_ns, period_ns)
    |
I.MX6ULL PWM1 controller → GPIO (backlight)
    |
LCD backlight brightness changes
```

The backlight brightness is controlled by writing 0~7 to the sysfs file.
The program uses a sine wave to smoothly transition between brightness
levels, creating a breathing effect.

## Repository Structure

```
pwm/backlight_breathing/
├── backlight_breathing.c   # Userspace C program
└── README.md               # This file
```

## Build

Cross-compile for ARM:

```bash
arm-linux-gnueabihf-gcc -o backlight_breathing backlight_breathing.c -lm
```

## Deploy & Run

```bash
# Upload to board
scp backlight_breathing root@<BOARD_IP>:/home/root/

# Run (Ctrl+C to stop)
./backlight_breathing
```

## DTS Requirement

The backlight is configured in the device tree. The PWM controller must
be enabled:

```dts
backlight {
    compatible = "pwm-backlight";
    pwms = <&pwm1 0 5000000>;
    brightness-levels = <0 4 8 16 32 64 128 255>;
    default-brightness-level = <7>;
    status = "okay";
};

&pwm1 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_pwm1>;
    status = "okay";
};
```

## Manual Test (without the program)

```bash
# Control backlight brightness directly via sysfs
cat /sys/class/backlight/backlight/brightness   # check current
echo 0 > /sys/class/backlight/backlight/brightness  # off
echo 7 > /sys/class/backlight/backlight/brightness  # brightest
```
