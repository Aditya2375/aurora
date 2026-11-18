#ifndef AURORA_IDT_H
#define AURORA_IDT_H

#include <stdint.h>

struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} __attribute__((packed));

typedef void (*isr_handler_t)(struct registers *regs);

void idt_init(void);
void register_interrupt_handler(uint8_t n, isr_handler_t handler);

#define IRQ0  32
#define IRQ1  33
#define IRQ12 44

void irq_enable(uint8_t irq);
void irq_disable(uint8_t irq);

#endif
