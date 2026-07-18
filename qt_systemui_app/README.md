# Timeset — systemUI Qt App Example

A date/time setting application integrated into the systemUI desktop
environment on I.MX6ULL. Demonstrates Qt Quick (QML) + C++ integration with
the systemUI framework.

## Hardware

| Component | Specification |
|-----------|---------------|
| **SoC** | NXP I.MX6ULL, Cortex-A7 @ 792MHz |
| **RAM** | 512MB DDR3L |
| **Storage** | 8GB eMMC |
| **Display** | 7-inch RGB LCD, 1024x600, 16bpp RGB565 |
| **Touch** | Goodix GT911 (I2C) |
| **Ethernet** | 10/100Mbps, static IP (192.168.1.100) |
| **Serial** | CH340 USB-UART, 115200 baud |

## Development Environment

| Tool | Details |
|------|---------|
| **Host** | Windows 10 + WSL2 (Ubuntu 24.04) |
| **Cross-compiler** | NXP i.MX Yocto SDK (`arm-poky-linux-gnueabi-g++`) |
| **Qt version** | 5.12.9 (ARM, included in Yocto SDK) |
| **SDK path** | `/opt/fsl-imx-x11/4.1.15-2.1.0/` |
| **Serial access** | usbipd-win (COM4 -> WSL2 `/dev/ttyUSB0`) |
| **SSH** | `ssh root@192.168.1.100` (password: `root`) |

## What It Demonstrates

```
User taps "timeset" icon on systemUI desktop
    |
    v
systemUI launches the app via .cfg configuration
    |
    v
QML displays current system date/time
    |
    v
User adjusts year/month/day/hour/minute/second via SpinBox
    |
    v
User taps "Set Time" button
    |
    v
C++ TimeControl calls `busybox date MMDDhhmmYYYY.ss` to set system time
    |
    v
Updated time displayed on screen
```

Key concepts:
- **systemUI integration**: App appears on desktop via `apk1.cfg` configuration file
- **Qt Quick (QML) + C++**: UI in QML, system operations in C++
- **Client module**: Uses `systemuicommonapiclient` for systemUI communication
- **System time setting**: Uses BusyBox `date` command via `system()`

## Repository Structure

```
qt_systemui_app/
├── timeset/
│   ├── timeset.pro              # qmake project file
│   ├── main.cpp                 # C++ entry, QML engine setup
│   ├── main.qml                 # QML Window with Client integration
│   ├── AppMainBody.qml          # Main UI (SpinBox + Set Time button)
│   ├── qml.qrc                  # QML resource file
│   └── timecontrol.h            # C++ TimeControl (read/set system time)
├── client/                      # systemUI communication module (dependency)
│   ├── client.pri               # qmake include file
│   ├── systemuicommonapiclient.h/.cpp
│   ├── Client.qml / Dock.qml    # systemUI QML components
│   ├── CustomRectangle.qml / CustomSwitch.qml
│   └── common.qrc               # shared resources
└── README.md                    # This file
```

## Build

### Prerequisites

The NXP Yocto SDK with Qt5 must be installed at `/opt/fsl-imx-x11/4.1.15-2.1.0/`.
If not installed, run the SDK installer:

```bash
chmod +x /opt/fsl-imx-x11-glibc-x86_64-meta-toolchain-qt5-cortexa7hf-neon-toolchain-4.1.15-2.1.0_20241230.sh
/opt/fsl-imx-x11-glibc-x86_64-meta-toolchain-qt5-cortexa7hf-neon-toolchain-4.1.15-2.1.0_20241230.sh
```

### Compile

```bash
# 1. Set up cross-compilation environment
source /opt/fsl-imx-x11/4.1.15-2.1.0/environment-setup-cortexa7hf-neon-poky-linux-gnueabi

# 2. Enter project directory
cd /opt/systemui-source/timeset

# 3. Build
qmake && make -j16
```

The compiled ARM binary will be at `timeset`.

### Deploy to Board

```bash
scp timeset root@192.168.1.100:/opt/ui/src/apps/timeset
```

## Deploy Paths (on the board)

| Item | Path |
|------|------|
| **Executable** | `/opt/ui/src/apps/timeset` |
| **Icon** | `/opt/ui/src/appicons/timeset.png` |
| **Desktop config** | `/opt/ui/src/ATK-IMX6U/apk1.cfg` (append: `appicons/timeset.png 时间设置 timeset`) |

## Dependencies

- `systemUI` desktop environment (running on the board)
- `client/` module (included via `../client/client.pri`)
- Qt5 modules: `QtQuick`, `QtQuickControls2`, `QtWidgets`, `QtRemoteObjects`
