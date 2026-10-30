; Aurora - kernel entry point
[BITS 32]
[SECTION .text]

extern kmain
extern _bss_start
extern _bss_end

global _start
_start:
    mov esp, 0x00200000             ; 64 KiB stack below 2 MiB
    xor ebp, ebp

    mov edi, _bss_start             ; zero .bss
    mov ecx, _bss_end
    sub ecx, edi
    add ecx, 3
    shr ecx, 2
    xor eax, eax
    rep stosd

    call kmain

.hang:
    cli
    hlt
    jmp .hang
