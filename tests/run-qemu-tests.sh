#!/bin/sh
# Boot Aurora in QEMU and assert the kernel comes up and its shell works:
# banner markers and boot self-tests on the serial console, then the mem
# and uptime commands typed for real through the QEMU monitor (sendkey
# goes through the emulated PS/2 controller, exactly like a keyboard).
# Exits non-zero on any miss.
set -e
cd "$(dirname "$0")/.."
IMG=${IMG:-build/aurora.img}
SERIAL=$(mktemp)
QEMU=${QEMU:-qemu-system-x86_64}

if [ ! -f "$IMG" ]; then
    echo "run 'make' first" >&2
    exit 1
fi

type_cmd() {   # one sendkey per character, through the monitor
    for ch in $1; do
        printf 'sendkey %s\n' "$ch"
        sleep 0.3
    done
}

( sleep 6
  type_cmd "m e m ret"
  sleep 2
  type_cmd "u p t i m e ret"
  sleep 2
  printf 'quit\n'
) | timeout 30 "$QEMU" \
    -drive format=raw,file="$IMG" -m 128M \
    -display none -serial "file:$SERIAL" -monitor stdio \
    -nic none >/dev/null 2>&1 || true

fail=0
check() {
    if ! grep -qF "$1" "$SERIAL"; then
        echo "MISSING: $1" >&2
        fail=1
    fi
}
check "AURORA BOOT SERIAL"
check "BOOTTEST all PASS"
check "KERNEL READY"
check "aurora:~\$ "
check "e820 total memory"       # mem ran
check "heap integrity    : ok"
check "up 0:00:"                # uptime ran
if grep -q "EXCEPTION" "$SERIAL"; then
    echo "kernel exception during boot" >&2
    fail=1
fi
rm -f "$SERIAL"
[ $fail -eq 0 ] && echo "QEMU BOOT TEST PASSED" || exit 1
