#include "timer.h"
#include "idt.h"
#include "ports.h"

static volatile uint64_t ticks = 0;

static void timer_callback(struct registers *regs) {
    (void)regs;
    ticks++;
}

void timer_init(void) {
    uint32_t divisor = 1193180 / TIMER_HZ;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    register_interrupt_handler(IRQ0, timer_callback);
    irq_enable(0);
}

uint64_t timer_ticks(void) {
    return ticks;
}

void timer_sleep(uint64_t ms) {
    uint64_t target = ticks + (ms * TIMER_HZ + 999) / 1000;
    while (ticks < target)
        __asm__ volatile ("hlt");
}
