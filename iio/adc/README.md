# I.MX6ULL ADC — Built-in ADC Driver

This chapter covers the I.MX6ULL's internal 12-bit ADC (Analog-to-Digital
Converter). Like many SoC peripherals, the ADC driver is already built into
the kernel — no driver code is needed.

## Hardware

| Parameter | Value |
|-----------|-------|
| **Resolution** | 12-bit (0 ~ 4095) |
| **Channels** | 4 (single-ended) |
| **Reference voltage** | 3.3V |
| **Conversion time** | Configurable |
| **Driver** | `CONFIG_VF610_ADC=y` (built-in) |

## Enabling the ADC

The ADC node in the Device Tree is disabled by default:

```dts
// imx6ull.dtsi (NXP provides this — default disabled)
adc@02198000 {
    compatible = "fsl,imx6ul-adc", "fsl,vf610-adc";
    reg = <0x2198000 0x4000>;
    interrupts = <0x0 0x64 0x4>;
    clocks = <&adc_clk>;
    num-channels = <0x2>;
    clock-names = "adc";
    status = "disabled";        /* ← change to "okay" */
};
```

Change `status = "disabled"` to `status = "okay"`, recompile the DTB,
and reboot.

## Reading ADC Data

No application is needed — the ADC uses the IIO framework and is accessed
via standard sysfs files:

```bash
# List IIO devices
ls /sys/bus/iio/devices/

# Read raw ADC value (channel 1)
cat /sys/bus/iio/devices/iio:device0/in_voltage1_raw

# Read scale factor (µV per digit)
cat /sys/bus/iio/devices/iio:device0/in_voltage_scale

# Convert to voltage: raw × scale
```

## Sample Output

```
$ for i in 1 2 3 4 5 6 7 8 9 10; do
>   cat /sys/bus/iio/devices/iio:device0/in_voltage1_raw
>   usleep 200000
> done
7
8
8
15
10
3
10
7
10
5
```