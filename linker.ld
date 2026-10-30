OUTPUT_FORMAT("elf32-i386")
ENTRY(_start)
SECTIONS {
    . = 0x00100000;
    .text : {
        */entry.asm.o(.text*)
        *(.text*)
    }
    . = ALIGN(16);
    .rodata : { *(.rodata*) }
    . = ALIGN(16);
    .data : { *(.data*) }
    . = ALIGN(16);
    _bss_start = .;
    .bss : { *(.bss*) *(COMMON) }
    . = ALIGN(16);
    _bss_end = .;
}
