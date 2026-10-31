# Aurora architecture

## Boot sequence

1. **BIOS** loads sector 0 (LBA 0) of the disk to `0000:7C00` and jumps in.
2. **Boot sector** (`boot/boot.asm`, 512 bytes) checks int 13h EDD support
   and packet-reads the whole blob that follows it - 8 sectors of stage-2
   loader plus the kernel image - to physical `0x10000`. Then a far jump
   into the loader. Kernel size is baked in at build time by the Makefile.
3. **Loader** (`boot/loader.asm`) enables A20 (verifies after each method,
   refuses to boot rather than run with a half-addressable machine), stores
   up to 64 E820 entries at physical `0x5000` with the count at `0x4FF0`,
   installs a flat 2-descriptor GDT, enters protected mode, copies the
   kernel image to `0x100000`, and enables identity paging over the low
   128 MiB using 4 MiB pages (page directory at `0x50000`). Interrupts stay
   masked from this point until the kernel has its own IDT.
4. **Kernel entry** (`kernel/entry.asm`) sets a 64 KiB stack at
   `0x1F0000-0x200000`, zeroes `.bss`, calls `kmain`.
5. **kmain** brings up serial, VGA, GDT, IDT + PIC remap, physical memory,
   heap, timer, keyboard; runs boot self-tests; starts the shell.

## Physical memory map

| Range                 | Use                                    |
|-----------------------|----------------------------------------|
| `0x0000 - 0x4EFF`     | IVT, BIOS data area                    |
| `0x4FF0 - 0x5000`     | E820 entry count                       |
| `0x5000 - 0x5600`     | E820 table (64 entries x 24 bytes)     |
| `0x7C00 - 0x7E00`     | boot sector (transient)                |
| `0x10000 - 0x11000`   | stage-2 loader (transient)             |
| `0x50000 - 0x51000`   | page directory                         |
| `0x100000 - end`      | kernel image                           |
| `0x1F0000 - 0x200000` | kernel stack (reserved in the PMM)     |
| everything else       | managed by the physical page allocator |

The PMM reserves the low 1 MiB, the page directory, the kernel image, and
the kernel stack, then hands out 4 KiB frames from a bitmap over the low
512 MiB. The heap claims contiguous runs from the PMM and sub-allocates
them with a best-fit free list that splits and coalesces blocks.

## Interrupts

The IDT covers CPU exceptions 0-31 and IRQs 0-15 remapped to vectors 32-47.
Exception handlers print a register dump and halt; IRQs acknowledge both
PICs and dispatch to registered handlers (timer tick, keyboard buffer).

## Debugging

`-serial stdio` gives the full kernel log. `KERNEL EXCEPTION` frames print
int number, error code and all general registers.
