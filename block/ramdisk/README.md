# RAM Disk — Linux Block Device Driver Demo

Demonstrates two approaches to writing a Linux block device driver
using a RAM disk (memory-backed storage simulated via DDR3).

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC + 32GB SD card |
| **RAM disk size** | 2MB (allocated from DDR3) |
| **Ethernet** | 10/100Mbps, APIPA (169.254.x.x) |
| **Serial** | CH340 USB-UART, 115200 baud |

## Development Environment

| Tool | Details |
|------|---------|
| **Host** | Windows 10 + WSL2 (Ubuntu 24.04) |
| **Cross-compiler** | Linaro GCC 5.3.1 |
| **Kernel source** | `linux-imx-4.1.15-2.1.0-g3dc0a4b-v2.7` (factory release) |
| **Build flags** | `ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-` |

## Two Methods Compared

```
Method 1 — Request Queue:               Method 2 — Make Request:
─────────────────────────               ────────────────────────
blk_init_queue()                         blk_alloc_queue()
(request_fn)                             blk_queue_make_request()

I/O requests flow:                       I/O requests flow:
  File system → bio                       File system → bio
    → I/O scheduler                         → make_request_fn
    → request_fn                              → memcpy directly
    → ramdisk_transfer()
```

| Aspect | Method 1 (wreq) | Method 2 (noreq) |
|--------|----------------|-----------------|
| **I/O scheduler** | Yes (merges/sorts requests) | No (bio goes straight through) |
| **Complexity** | Higher (request → bio → bvec) | Lower (bio → bvec directly) |
| **Best for** | Mechanical HDDs | RAM / Flash / SSD |
| **Init function** | `blk_init_queue()` | `blk_alloc_queue()` + `blk_queue_make_request()` |

## Repository Structure

```
block/
└── ramdisk/
    ├── ramdisk_wreq.c      # Method 1: Request Queue
    ├── ramdisk_noreq.c     # Method 2: Make Request (no scheduler)
    └── README.md
```

## Build

```bash
# Build both drivers
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

## Deploy & Test

```bash
# Load method 1
scp ramdisk_wreq.ko root@<BOARD_IP>:/home/root/
insmod /home/root/ramdisk_wreq.ko
mkfs.ext4 /dev/ramdisk_wreq
mount /dev/ramdisk_wreq /mnt/
echo "Hello RAM disk" > /mnt/test.txt
cat /mnt/test.txt
umount /mnt/

# Load method 2 (unload method 1 first)
rmmod ramdisk_wreq
insmod /home/root/ramdisk_noreq.ko
mkfs.ext4 /dev/ramdisk_noreq
mount /dev/ramdisk_noreq /mnt/
echo "Hello RAM disk2" > /mnt/test_2.txt
cat /mnt/test_2.txt
```

## Block Device Architecture

```
Application: write() / read()
    │
VFS (Virtual File System)
    │
File system (ext4)
    │
Page Cache (DDR3)
    │
I/O Scheduler (method 1 only)
    │
Block device driver (gendisk + request_fn / make_request_fn)
    │
Storage (DDR3 buffer)
```
