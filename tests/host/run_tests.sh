#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
OUT=${OUT:-build/tests}
mkdir -p "$OUT"
CFLAGS="-std=gnu11 -O2 -g -fno-builtin -fno-strict-aliasing -Wall -Wextra -Werror -Wno-stringop-truncation ${EXTRA_CFLAGS:-}"
cc $CFLAGS -Ikernel/lib tests/host/test_string.c kernel/lib/string.c -o "$OUT/test_string"
cc $CFLAGS -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Ikernel -Ikernel/lib tests/host/test_pmm_heap.c kernel/lib/string.c -o "$OUT/test_pmm_heap"
cc $CFLAGS -I. -Ikernel -Ikernel/lib tests/host/test_kprint.c -o "$OUT/test_kprint"
cc $CFLAGS -I. -Ikernel -Ikernel/lib tests/host/test_vga.c kernel/lib/string.c -o "$OUT/test_vga"
cc $CFLAGS -Ikernel -Ikernel/lib tests/host/test_fs.c kernel/fs.c kernel/lib/string.c -o "$OUT/test_fs"
for test in string pmm_heap kprint vga fs; do "$OUT/test_$test"; done
echo "ALL HOST TESTS PASSED"
