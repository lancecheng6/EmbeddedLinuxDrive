# KGDB Kernel Debugging on I.MX6ULL

Setup and workflow for debugging the Linux kernel on the I.MX6ULL board using KGDB over serial.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Display** | 7-inch RGB LCD, 1024x600, 16bpp RGB565 |
| **Touch** | Goodix GT911 (I2C) |
| **Serial** | CH340 USB-UART, 115200 baud, COM7 |
| **Board IP** | `192.168.1.100` (static, set in u-boot) |

## Development Environment

| Tool | Details |
|------|---------|
| **Host** | Windows 10 + WSL2 (Ubuntu 20.04) |
| **Cross-compiler** | Linaro GCC 7.3.1-2018.05 |
| **Kernel source** | `linux-imx-4.1.15-2.1.0-g3dc0a4b-v2.7` |
| **Build system** | Buildroot (`/root/buildroot`) |
| **GDB (Windows)** | Linaro GDB 8.1 (`arm-linux-gnueabihf-gdb.exe`) |
| **Serial debug** | MobaXterm (COM7, 115200) |

## Kernel Configuration Changes

All modifications are in `/root/buildroot/linux/linux.mk`.

### Kconfig fixups added

| Option | Action | Purpose |
|--------|--------|---------|
| `CONFIG_KGDB=y` | Enable | KGDB stub |
| `CONFIG_KGDB_SERIAL_CONSOLE=y` | Enable | KGDB over UART |
| `CONFIG_KGDB_KDB=y` | Enable | KDB shell (fallback) |
| `CONFIG_DEBUG_INFO=y` | Enable | Debug symbols in vmlinux |
| `CONFIG_IMX2_WDT=n` | Disable | Prevent watchdog reset during breakpoints |

### Build command change

`BUILD_CMDS` changed from `make all` to `make zImage` to skip broken DTB compilation.

## Repository Files

```
kgdb_debug/
├── README.md                  ← This file
├── arm-kgdb-target.xml        GDB target description (26 registers)
├── arm-linux-gnueabihf-gdb.exe  GDB 8.1 for Windows (93MB)
├── vmlinux                    Kernel with debug symbols (153MB)
├── zImage                     Compressed kernel image (6.7MB)
├── kgdb-kernel.config         Full kernel .config
├── linux.mk                   Buildroot build script with KGDB fixups
└── kernel_factory.tar.gz      Original factory kernel source (300MB)
```

## Prerequisites

1. **USB-UART driver** — CH340 driver installed, board appears as COM7
2. **MobaXterm** — Serial session on COM7, 115200 baud
3. **Board powered** — Connected via Ethernet (192.168.1.100) and serial

## Step-by-Step KGDB Workflow

### Step 1 — Set boot parameters (one-time)

Interrupt u-boot at boot (press any key) and run:

```bash
setenv mmcargs setenv bootargs console=${console},${baudrate} root=${mmcroot} kgdboc=ttymxc0,115200
saveenv
boot
```

Verify after boot:

```bash
cat /proc/cmdline
# Expected: console=ttymxc0,115200 root=/dev/mmcblk1p2 rootwait rw kgdboc=ttymxc0,115200
```

### Step 2 — Trigger KGDB

On the board console (MobaXterm):

```bash
echo g > /proc/sysrq-trigger
```

Expected output:

```
[   21.137746] sysrq: SysRq : DEBUG
Entering kdb (current=0x94330a00, pid 699) on processor 0 due to Keyboard Entry
[0]kdb>
```

### Step 3 — Switch KDB to GDB stub

At the `[0]kdb>` prompt:

```
kgdb
```

The board will display:

```
KGDB: waiting for remote connection...
```

The board now freezes — **close MobaXterm** to release COM7.

### Step 4 — Connect GDB from Windows

Open PowerShell and run:

```powershell
C:\kgdb\arm-linux-gnueabihf-gdb.exe C:\kgdb\vmlinux
```

At the `(gdb)` prompt, enter:

```
set serial baud 115200
set remotetimeout 3
set tdesc filename C:/kgdb/arm-kgdb-target.xml
target remote \\.\COM7
```

On success you will see:

```
0x800b5274 in arch_kgdb_breakpoint () at ./arch/arm/include/asm/outercache.h:142
```

### Step 5 — Debug

Enter these GDB commands:

| Command | Description |
|---------|-------------|
| `bt` | Print backtrace (call stack) |
| `list` | Show source code around current PC |
| `info registers` | Dump all registers |
| `p $r0` | Print register r0 value |
| `p variable_name` | Print kernel variable |
| `break function_name` | Set breakpoint |
| `c` or `continue` | Resume kernel execution |
| `s` or `step` | Single-step one instruction |
| `n` or `next` | Step over function call |
| `Ctrl+C` | Interrupt kernel (re-enter KGDB) |
| `quit` | Exit GDB (kernel continues) |