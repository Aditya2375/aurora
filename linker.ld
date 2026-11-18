OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS {
    . = 0x00100000;
    _text_start = .;
    .text : { */entry.asm.o(.text*) *(.text*) }
    . = ALIGN(4096);
    .rodata : { *(.rodata*) }
    . = ALIGN(4096);
    _ro_end = .;
    .data : { *(.data*) }
    . = ALIGN(4096);
    _bss_start = .;
    .bss : { *(.bss*) *(COMMON) }
    . = ALIGN(4096);
    _bss_end = .;
    ASSERT(_bss_end <= 0x1EF000, "kernel overlaps stack guard")
    /DISCARD/ : { *(.note.GNU-stack) *(.comment) }
}
