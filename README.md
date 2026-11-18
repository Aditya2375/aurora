# Aurora

A 32-bit x86 operating system written from scratch in C and NASM assembly:
two-stage bootloader, protected mode, 4 KiB paging with write protection,
interrupt-driven drivers, a checked allocator, a RAM filesystem and a shell.
No GRUB, no standard library, no starter code.

**[Boot Aurora in your browser](https://aditya2375.github.io/aurora/)** - the real disk
image running in the v86 x86 emulator (WebAssembly). Click the screen and type `help`.

## What it does

- **Boot:** BIOS boot sector, then a stage-2 loader that enables A20, reads the E820
  memory map, enters protected mode and starts the kernel. The loader halts with a
  message on a malformed or truncated E820 map instead of guessing.
- **Memory protection:** 4 KiB supervisor paging with CR0.WP. Kernel code and
  constants are read-only, page 0 is unmapped (null guard), and a guard page sits
  below the kernel stack. `check` triggers three real page faults on purpose
  (null read, guard read, write to code) and shows the hardware caught each.
- **Allocators:** E820-driven page allocator (firmware-reserved ranges win over
  overlapping usable ranges) and a checked kernel heap (pointer validation,
  no coalescing across gaps, freed and new memory zeroed).
- **Filesystem:** bounded RAM filesystem with directories, paths (`/`, `.`, `..`),
  48 nodes, 1023 bytes per file, and a read-only `/sys`.
- **Shell:** `help about mem check uptime echo history pwd ls cd tree stat cat mkdir touch rm write append color clear halt`, with up/down history.
- Everything printed also goes to COM1, which the tests assert on.

## Security: what is and is not hardened

Hardened (tested):
- Read-only code and constants, null page and stack guard, exact-match handling of
  the three deliberate diagnostic faults (any other fault halts the machine).
- Bounded input everywhere: command line, names, paths, file size, node count,
  allocation size. Formatter and filesystem are fuzz/state-machine tested.
- E820 loader fails closed on bad firmware data.

Not provided: user processes, ring-3 isolation, NX, ASLR, secure boot, networking,
accounts, encryption, persistent storage. Every command runs in ring 0.

Known limits, stated plainly:
- A kernel stack overflow ends in a CPU reset (double or triple fault), not a
  diagnosis. The guard page only stops it from silently corrupting memory.
- The page tables live in writable kernel data, so a write bug in ring-0 code
  could change them and disable protection. Protection is against accidents and
  bugs in the checked paths, not against code that already runs in ring 0.
- If firmware returns an error after a valid last entry while more entries are
  pending, boot halts. That favors safety over availability.
- The browser and v86 are part of the trusted base. Files vanish on reboot, so
  never put secrets in the demo.

## Build

Requires `nasm`, `gcc` (with `-m32`), `ld`, `objcopy`, `make`.

```sh
make            # produces build/aurora.img
./scripts/run.sh
```

## Test

```sh
./tests/host/run_tests.sh      # host unit and state-machine tests
python3 tests/e820.py          # real loader assembly run in Unicorn against bad BIOS replies
python3 tests/integration.py   # boots QEMU at 16/32/128/256 MiB, types real commands
```

CI runs the host suite (also under ASan/UBSan) and the QEMU boot test. Not tested:
bare metal and any hardware beyond QEMU's and v86's emulated devices.

## Third-party

The browser demo bundles v86 (BSD-2-Clause) and SeaBIOS/VGABIOS firmware (LGPL),
unmodified. Licenses, build config and SeaBIOS rel-1.16.2 source are in
`docs/firmware/`. Aurora itself is MIT licensed.
