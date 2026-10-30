; -----------------------------------------------------------------------------
; Aurora - stage 2 loader
;
; Runs at physical 0x10000, entered with a far jump from the boot sector.
; Real mode: enables A20 and records the E820 memory map. Then: flat GDT,
; protected mode, kernel copy to 1 MiB, identity paging over the low 128 MiB
; (4 MiB pages), and a far jump to the kernel entry point.
; -----------------------------------------------------------------------------
[BITS 16]
[ORG 0]

%include "defines.inc"

%define LOAD_PHYS  0x10000
%define PHYS(x)    (LOAD_PHYS + (x))
%define E820_SEG   0x0500          ; entries at physical 0x5000
%define E820_MAX   64
%define E820_COUNT 0x4FF0          ; physical, word
%define PD_PHYS    0x00050000
%define KERNEL_DST 0x00100000
%define STACK32    0x00090000

start:
    cli
    mov ax, 0x1000
    mov ds, ax
    mov es, ax
    xor ax, ax
    mov ss, ax
    mov sp, 0xF000

    call a20_enable
    call e820_load

    cli                             ; BIOS int 15h can re-enable interrupts;
                                    ; keep them off until the kernel has an IDT
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, 1
    mov cr0, eax
    jmp dword 0x08:PHYS(pm32)  ; operand-size override: full 32-bit offset

; ---------------------------------------------------------------------------
; A20: verify after each method; disabled A20 wraps odd-megabyte addresses.
; ---------------------------------------------------------------------------
a20_enable:
    call a20_test
    jnz  .done
    mov  ax, 0x2401                 ; BIOS A20 gate
    int  0x15
    call a20_test
    jnz  .done
    in   al, 0x92                   ; fast A20 port
    or   al, 2
    and  al, 0xFE
    out  0x92, al
    call a20_test
    jnz  .done
    call kbd_wait                   ; 8042 controller method
    mov  al, 0xAD
    out  0x64, al
    call kbd_wait
    mov  al, 0xD0
    out  0x64, al
.kw2:
    in   al, 0x64
    test al, 1
    jz   .kw2
    in   al, 0x60
    push ax
    call kbd_wait
    mov  al, 0xD1
    out  0x64, al
    call kbd_wait
    pop  ax
    or   al, 2
    out  0x60, al
    call kbd_wait
    mov  al, 0xAE
    out  0x64, al
    call kbd_wait
    call a20_test
    jnz  .done
    mov  si, msg_a20
    jmp  print_hang
.done:
    ret

kbd_wait:
    in   al, 0x64
    test al, 2
    jnz  kbd_wait
    ret

; Returns ZF clear when A20 is enabled.
a20_test:
    push ds
    push es
    push si
    push di
    xor  ax, ax
    mov  ds, ax
    mov  si, 0x0500
    mov  ax, 0xFFFF
    mov  es, ax
    mov  di, 0x0510
    mov  al, [si]
    mov  ah, [es:di]
    mov  byte [si], 0x00
    mov  byte [es:di], 0xFF
    cmp  byte [si], 0xFF            ; wrapped? then A20 is off
    mov  [si], al
    mov  [es:di], ah
    pop  di
    pop  si
    pop  es
    pop  ds
    ret

; ---------------------------------------------------------------------------
; E820 memory map: up to 64 entries at physical 0x5000, count word at 0x4FF0.
; ---------------------------------------------------------------------------
e820_load:
    push es
    mov  ax, E820_SEG
    mov  es, ax
    xor  di, di
    xor  ebx, ebx
    xor  ebp, ebp
.next:
    mov  eax, 0x0000E820
    mov  ecx, 24
    mov  edx, 0x534D4150
    int  0x15
    jc   .done
    cmp  eax, 0x534D4150
    jne  .done
    test ecx, ecx
    jz   .skip
    cmp  ecx, 20
    ja   .attr_ok
    mov  dword [es:di+20], 1        ; ACPI 1.0 entry: mark as valid
.attr_ok:
    inc  bp
    add  di, 24
    cmp  bp, E820_MAX
    jae  .done
.skip:
    test ebx, ebx
    jnz  .next
.done:
    xor  ax, ax
    mov  es, ax
    mov  [es:E820_COUNT], bp
    pop  es
    ret

print_hang:
.p: lodsb
    test al, al
    jz  .h
    mov ah, 0x0E
    xor bh, bh
    int 0x10
    jmp .p
.h: cli
    hlt
    jmp .h

msg_a20 db "Aurora: could not enable A20", 0

align 8
gdt:
    dq 0                            ; 0x00 null
.code:                              ; 0x08 kernel code
    dw 0xFFFF, 0
    db 0, 0x9A, 0xCF, 0
.data:                              ; 0x10 kernel data
    dw 0xFFFF, 0
    db 0, 0x92, 0xCF, 0
gdt_end:
gdt_desc:
    dw gdt_end - gdt - 1
    dd PHYS(gdt)

[BITS 32]
pm32:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, STACK32

    ; Copy the kernel image to 1 MiB.
    mov esi, LOAD_PHYS + LOADER_SECTORS*512
    mov edi, KERNEL_DST
    mov ecx, KERNEL_SIZE
    shr ecx, 2
    rep movsd

    ; Identity-map the low 128 MiB with 4 MiB pages where the CPU has PSE
    ; (every CPU since the Pentium). Without PSE, run with paging off: the
    ; kernel is linked at its physical address, so both paths work.
    mov eax, 1
    cpuid
    test edx, 1 << 3
    jz   .no_paging

    mov edi, PD_PHYS
    xor eax, eax
    mov ecx, 1024
    rep stosd
    mov edi, PD_PHYS
    mov eax, 0x00000083             ; 4 MiB page, PS | RW | present
    mov ecx, 32
.fill:
    mov [edi], eax
    add eax, 0x00400000
    add edi, 4
    loop .fill

    mov eax, cr4
    or  eax, 1 << 4                 ; CR4.PSE
    mov cr4, eax
    mov eax, PD_PHYS
    mov cr3, eax
    mov eax, cr0
    or  eax, 1 << 31                ; CR0.PG
    mov cr0, eax
.no_paging:
    jmp 0x08:KERNEL_DST

times (LOADER_SECTORS*512)-($-$$) db 0
