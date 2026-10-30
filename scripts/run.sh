#!/bin/sh
# Boot Aurora in QEMU. Graphics window if a display exists, serial always.
IMG=${1:-build/aurora.img}
QEMU=${QEMU:-qemu-system-x86_64}
EXTRA=""
if [ -z "$DISPLAY" ]; then
    EXTRA="-display none"
fi
exec "$QEMU" -drive format=raw,file="$IMG" -m 128M -serial stdio $EXTRA
