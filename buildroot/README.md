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
| **Host** | Windows 10 + WSL2 (Ubuntu 20.04 LTS) |
| **Cross-compiler** | Linaro ARM gnueabihf (external, Buildroot) |
| **Kernel source** | `linux-imx-4.1.15-2.1.0-g3dc0a4b-v2.7` (factory release) |
| **Buildroot** | 2020.02.x (uses bison 3.4.2, compatible with kernel's DTC) |
| **Build flags** | `FORCE_UNSAFE_CONFIGURE=1 make -j4` |
| **Serial access** | usbipd-win (COM4 → WSL2 `/dev/ttyUSB0`) |

## Repository Structure

```
EmbeddedLinuxDrive/
└── buildroot/
    ├── configs/
    │   └── imx6ull_defconfig                             # Buildroot configuration
    ├── scripts/
    │   └── build_kernel.sh                               # Automated build script
    ├── output/
    │   ├── zImage                                        # Compiled kernel image
    │   ├── rootfs.ext4                                   # Root filesystem (Busybox, no desktop)
    │   ├── rootfs.tar                                    # Root filesystem tarball
    │   ├── imx6ull-14x14-emmc-7-1024x600-c.dtb           # Device tree blob
    │   └── u-boot.bin                                    # Bootloader
    └── README.md                                         # This file
```

## Build & Deploy

### Prepare kernel source tarball

```bash
cd /opt
tar czf ~/kernel_factory.tar.gz kernel_factory/
```

### Setup and build

```bash
# Download Buildroot 2020.02.x
cd ~
git clone -b 2020.02.x https://git.busybox.net/buildroot
cd buildroot

# Copy configuration
cp <path>/configs/imx6ull_defconfig configs/
make imx6ull_defconfig

# Build
FORCE_UNSAFE_CONFIGURE=1 make -j4
```

### Deploy to board

```bash
# Upload all images
scp output/images/zImage root@<BOARD_IP>:/home/root/
scp output/images/rootfs.ext4 root@<BOARD_IP>:/home/root/
scp output/images/imx6ull-14x14-ddr3-arm2.dtb root@<BOARD_IP>:/home/root/
scp output/images/u-boot.bin root@<BOARD_IP>:/home/root/

# Flash U-Boot
dd if=/home/root/u-boot.bin of=/dev/mmcblk1 bs=1K seek=1

# Flash kernel + DTB
mount /dev/mmcblk1p1 /mnt
cp /home/root/zImage /mnt/
cp /home/root/*.dtb /mnt/
umount /mnt

# Flash rootfs
dd if=/home/root/rootfs.ext4 of=/dev/mmcblk1p2 bs=1M
sync

# Reboot
reboot
```

## Notes

- **Why Buildroot 2020.02.x?** Newer Buildroot versions use bison 3.7+ which
  is incompatible with the kernel 4.1.15 DTC parser. Buildroot 2020.02.x
  uses bison 3.4.2, which works without patching.

- **Why Ubuntu 20.04?** Older Buildroot versions require specific host tool
  versions. Ubuntu 20.04 provides glibc 2.31 and python3 which are
  compatible with Buildroot 2020.02.x.

- **DTB compilation**: The factory kernel's DTS has a brace mismatch that
  causes errors with newer DTC versions. Use the pre-built DTB from the
  factory source, or fix the DTS before compiling.

- **PATH issue**: When running Buildroot from Ubuntu 20.04 WSL, the PATH
  may inherit special characters from the parent WSL. Use
  `env -i PATH="/usr/local/sbin:..." make` if needed.
