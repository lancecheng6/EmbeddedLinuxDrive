#!/bin/bash
# rebuild2.sh — Run inside Ubuntu 20.04 WSL
cd ~/buildroot || { echo "buildroot dir not found"; exit 1; }
make distclean 2>/dev/null
make imx6ull_defconfig 2>&1 | tail -3
sed -i 's|kernel_factory_fixed.tar.gz|kernel_factory.tar.gz|g' .config
grep TARBALL_LOCATION .config
echo "Starting build..."
env -i PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" HOME="/root" FORCE_UNSAFE_CONFIGURE=1 make -j4 2>&1
