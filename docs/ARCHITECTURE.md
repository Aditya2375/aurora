# Aurora architecture

1. **Boot sector** (`boot/boot.asm`): loads loader and kernel with EDD reads.
2. **Loader** (`boot/loader.asm`): A20, E820 (fail closed), protected mode, copies kernel to 1 MiB.
3. **Kernel entry** (`kernel/entry.asm`): stack at 0x1F0000-0x200000, zero .bss, `kmain`.
4. **kmain**: serial, VGA, GDT, IDT, `memory_init` (4 KiB paging, CR0.WP), page allocator, heap, timer, keyboard, RAM filesystem, boot self-tests (including live protection probes), shell.

## Memory
Identity mapped low 128 MiB in 4 KiB pages. `_text_start.._ro_end` read-only; page 0 and 0x1EF000 unmapped. Page tables are in .bss (writable by ring 0). Physical allocator manages the rest; firmware-reserved ranges override usable ones.

## Faults
Page faults are recovered only when EIP, CR2 and error code exactly match one of three diagnostic probes in `kernel/probes.asm`. Anything else prints a register dump and halts. A stack overflow into the guard page faults on a stack that cannot be used, so it ends in a double or triple fault and a reset.
