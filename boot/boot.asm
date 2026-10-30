; -----------------------------------------------------------------------------
; Aurora - stage 1 boot sector
;
; Loaded by the BIOS at 0000:7C00. Loads the stage 2 loader and the kernel
; image (the "blob", LBA 1 onwards) to physical 0x10000 using int 13h EDD
; packet reads, then jumps to the loader.
; -----------------------------------------------------------------------------
[BITS 16]
[ORG 0x7C00]

%include "defines.inc"

%define LOAD_SEG 0x1000
%define CHUNK    127              ; max sectors per EDD packet read

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drive], dl

    ; BIOS must support int 13h extensions (EDD). Every PC since the late
    ; 90s has these; refuse to boot with a clear message otherwise.
    mov ah, 0x41
    mov bx, 0x55AA
    int 0x13
    jc  .no_edd
    cmp bx, 0xAA55
    jne .no_edd
    test cl, 1
    jz  .no_edd

    mov word [remaining], BLOB_SECTORS
    mov dword [dap_lba],  1
    mov word  [dap_seg],  LOAD_SEG
    mov word  [dap_off],  0

.load_loop:
    mov ax, [remaining]
    test ax, ax
    jz  .done
    cmp ax, CHUNK
    jbe .count_ok
    mov ax, CHUNK
.count_ok:
    mov [dap_count], ax

    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc  .disk_error

    mov ax, [dap_count]
    sub [remaining], ax
    add word [dap_lba], ax
    adc word [dap_lba+2], 0
    shl ax, 5                     ; sectors * 32 paragraphs
    add [dap_seg], ax
    jmp .load_loop

.done:
    mov dl, [boot_drive]
    jmp LOAD_SEG:0x0000

.no_edd:
    mov si, msg_no_edd
    jmp print_hang
.disk_error:
    mov si, msg_disk
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

boot_drive db 0
remaining  dw 0

align 4
dap:
dap_size   db 0x10
dap_resv   db 0
dap_count  dw 0
dap_off    dw 0
dap_seg    dw 0
dap_lba    dq 0

msg_no_edd db "Aurora: BIOS has no EDD disk extensions", 0
msg_disk   db "Aurora: disk read error", 0

times 510-($-$$) db 0
dw 0xAA55
