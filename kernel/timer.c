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

/* ticks is 64-bit but the CPU is 32-bit: a plain read can catch the
   value halfway through an IRQ increment. Read until the high word is
   stable, the usual seqlock pattern, so callers never see a torn tick. */
uint64_t timer_ticks(void) {
    uint32_t hi, lo, hi2;
    do {
        hi = (uint32_t)(ticks >> 32);
        lo = (uint32_t)ticks;
        hi2 = (uint32_t)(ticks >> 32);
    } while (hi != hi2);
    return ((uint64_t)hi << 32) | lo;
}

void timer_sleep(uint64_t ms) {
    uint64_t target = timer_ticks() + (ms * TIMER_HZ + 999) / 1000;
    while (timer_ticks() < target)
        __asm__ volatile ("hlt");
}
