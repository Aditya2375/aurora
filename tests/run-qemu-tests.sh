#!/bin/sh
# Boot Aurora in QEMU and assert the kernel comes up: banner markers and
# boot self-tests on the serial console. Exits non-zero on any miss.
set -e
cd "$(dirname "$0")/.."
IMG=${IMG:-build/aurora.img}
SERIAL=$(mktemp)
QEMU=${QEMU:-qemu-system-x86_64}

if [ ! -f "$IMG" ]; then
    echo "run 'make' first" >&2
    exit 1
fi

( sleep 6; printf 'quit\n' ) | timeout 15 "$QEMU" \
    -drive format=raw,file="$IMG" -m 128M \
    -display none -serial "file:$SERIAL" -monitor stdio \
    -nic none >/dev/null 2>&1 || true

fail=0
for marker in "AURORA BOOT SERIAL" "BOOTTEST all PASS" "KERNEL READY" "aurora:~\$ "; do
    if ! grep -qF "$marker" "$SERIAL"; then
        echo "MISSING: $marker" >&2
        fail=1
    fi
done
if grep -q "EXCEPTION" "$SERIAL"; then
    echo "kernel exception during boot" >&2
    fail=1
fi
rm -f "$SERIAL"
[ $fail -eq 0 ] && echo "QEMU BOOT TEST PASSED" || exit 1
