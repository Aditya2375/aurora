[BITS 32]
section .text
global probe_read, probe_read_fault, probe_read_resume
global probe_write, probe_write_fault, probe_write_resume
probe_read:
    mov eax, [esp+4]
probe_read_fault:
    mov eax, [eax]
probe_read_resume:
    ret
probe_write:
    mov eax, [esp+4]
probe_write_fault:
    mov byte [eax], 0
probe_write_resume:
    ret
section .note.GNU-stack noalloc noexec nowrite progbits
