#include "idt.h"
#include "kprint.h"
#include "ports.h"
#include "string.h"

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

static struct idt_entry idt[256];
static struct idt_ptr idtp;
static isr_handler_t handlers[256];

static void idt_set(uint8_t n, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[n].base_low  = (uint16_t)(base & 0xFFFF);
    idt[n].base_high = (uint16_t)((base >> 16) & 0xFFFF);
    idt[n].sel       = sel;
    idt[n].zero      = 0;
    idt[n].flags     = flags;
}

static void pic_remap(void) {
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();
    outb(0x21, 0x20);          /* master: vectors 32-39 */
    io_wait();
    outb(0xA1, 0x28);          /* slave: vectors 40-47 */
    io_wait();
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();
    outb(0x21, 0xFF);          /* all IRQs masked until requested */
    io_wait();
    outb(0xA1, 0xFF);
    io_wait();
}

void irq_enable(uint8_t irq) {
    uint16_t port = irq < 8 ? 0x21 : 0xA1;
    uint8_t bit = irq < 8 ? irq : irq - 8;
    outb(port, inb(port) & (uint8_t)~(1 << bit));
}

void irq_disable(uint8_t irq) {
    uint16_t port = irq < 8 ? 0x21 : 0xA1;
    uint8_t bit = irq < 8 ? irq : irq - 8;
    outb(port, inb(port) | (uint8_t)(1 << bit));
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    memset(&idt, 0, sizeof(idt));
    memset(&handlers, 0, sizeof(handlers));

    pic_remap();

    idt_set(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set(31, (uint32_t)isr31, 0x08, 0x8E);

    idt_set(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set(47, (uint32_t)irq15, 0x08, 0x8E);

    /* every remaining vector gets a catch-all stub that reports its number */
    extern uint32_t isr_default_table[];
    for (int v = 48; v < 256; v++)
        idt_set((uint8_t)v, isr_default_table[v - 48], 0x08, 0x8E);

    __asm__ volatile ("lidt %0" : : "m"(idtp));
}

void register_interrupt_handler(uint8_t n, isr_handler_t handler) {
    handlers[n] = handler;
}

static const char *exception_names[32] = {
    "divide error", "debug", "non-maskable interrupt", "breakpoint",
    "overflow", "bound range exceeded", "invalid opcode", "no math coprocessor",
    "double fault", "coprocessor segment overrun", "invalid TSS", "segment not present",
    "stack-segment fault", "general protection fault", "page fault", "reserved",
    "x87 floating-point error", "alignment check", "machine check", "SIMD exception",
    "virtualization exception", "control protection exception", "reserved", "reserved",
    "reserved", "reserved", "reserved", "reserved", "hypervisor injection exception",
    "VMM communication exception", "security exception", "reserved",
};

void isr_handler(struct registers *regs) {
    if (regs->int_no < 32) {
        const char *name = exception_names[regs->int_no];
        if (regs->int_no == 3 && handlers[3]) {
            handlers[3](regs);
            return;
        }
        __asm__ volatile ("cli");
        kprintf_attr(ATTR_ERR, "\n*** KERNEL EXCEPTION: %s (int %d, err %d)\n",
                     name, regs->int_no, regs->err_code);
        kprintf_attr(ATTR_ERR, "    eip=%x cs=%x eflags=%x\n",
                     regs->eip, regs->cs, regs->eflags);
        kprintf_attr(ATTR_ERR, "    eax=%x ebx=%x ecx=%x edx=%x\n",
                     regs->eax, regs->ebx,
                     regs->ecx, regs->edx);
        kprintf_attr(ATTR_ERR, "    esi=%x edi=%x ebp=%x esp=%x\n",
                     regs->esi, regs->edi,
                     regs->ebp, regs->esp);
        kprintf_attr(ATTR_ERR, "    system halted.\n");
        for (;;)
            __asm__ volatile ("hlt");
    }
    if (handlers[regs->int_no])
        handlers[regs->int_no](regs);
    else if (regs->int_no >= 48) {
        /* catch-all: log each unexpected vector once, then keep going */
        static uint8_t seen[32];
        uint8_t n = regs->int_no;
        if (!(seen[n / 8] & (1u << (n % 8)))) {
            seen[n / 8] |= (uint8_t)(1u << (n % 8));
            kprintf_attr(ATTR_WARN, "unhandled interrupt %d\n", n);
        }
    }
}

void irq_handler(struct registers *regs) {
    if (regs->int_no >= 40)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
    if (handlers[regs->int_no])
        handlers[regs->int_no](regs);
}
