#!/bin/sh
# Build and run the host unit tests for the kernel's portable code.
# Flags mirror the freestanding kernel build: -fno-builtin matters, gcc's
# loop-idiom recognition would otherwise turn memset's loop into a
# recursive call to memset.
set -e
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/tests}
mkdir -p "$OUT"

CFLAGS="-std=gnu11 -O2 -g -fno-builtin -fno-strict-aliasing -Wall -Wextra -Werror -Wno-stringop-truncation"

cc $CFLAGS -Ikernel/lib \
    tests/host/test_string.c kernel/lib/string.c -o "$OUT/test_string"

cc $CFLAGS -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Ikernel -Ikernel/lib \
    tests/host/test_pmm_heap.c kernel/lib/string.c -o "$OUT/test_pmm_heap"

cc $CFLAGS -I. -Ikernel -Ikernel/lib \
    tests/host/test_kprint.c -o "$OUT/test_kprint"

cc $CFLAGS -I. -Ikernel -Ikernel/lib \
    tests/host/test_vga.c kernel/lib/string.c -o "$OUT/test_vga"

"$OUT/test_string"
"$OUT/test_pmm_heap"
"$OUT/test_kprint"
"$OUT/test_vga"
echo "ALL HOST TESTS PASSED"
