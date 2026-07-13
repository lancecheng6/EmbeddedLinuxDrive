# USB Mass Storage — List Drive Contents

A userspace C program that reads the top-level contents of a mounted
USB drive (or any mounted filesystem) and displays them like `ls -la`.

This demonstrates that once Linux has enumerated and mounted a USB mass
storage device, it becomes a regular directory — accessible via
standard POSIX APIs (`opendir`, `readdir`, `stat`).

## Repository Structure

```
usb/list_usb/
├── usb_list.c        # C source code
├── USBContent.txt    # Sample output (Kingston DataTraveler 123GB)
└── README.md
```

## How It Works

The kernel handles all USB initialization automatically:

```
USB drive inserted
    │
Linux kernel:
  ├── USB enumeration
  ├── Load usb-storage driver
  ├── Create /dev/sda (block device)
  ├── Detect partitions (sda1, sda2)
  └── Mount filesystem → /run/media/sda2
                             │
Your C program:              │
  opendir("/run/media/sda2") │
  readdir()                  │
  stat()                     ▼
                    Read like any directory
```

## Build

Cross-compile for ARM:

```bash
arm-linux-gnueabihf-gcc -o usb_list usb_list.c
```

## Usage

```bash
# Deploy to board
scp usb_list root@<BOARD_IP>:/home/root/

# SSH into the board and run
ssh root@<BOARD_IP>
./usb_list /run/media/sda2

# Or run directly via SSH
ssh root@<BOARD_IP> "./usb_list /run/media/sda2"
```

## Sample Output

```
USB Drive: /run/media/sda2
========================================
drwxrwx---   4 root root    16384 Jan 01 00:00 .
drwxr-xr-x   6 root root      120 Dec 05 08:17 ..
drwxrwx---   4 root root     2048 Nov 20 12:17 EFI
-rwxrwx---   1 root root      177 Jan 29 12:06 README.txt
drwxrwx---   2 root root     2048 Jun 04 23:42 .fseventsd

Subdirectories: 4
Files: 1
```

## Key Takeaway

> **USB mass storage devices are just regular directories in Linux.**
> The kernel handles all the USB complexity — your application just
> reads files and directories like any other mounted filesystem.
