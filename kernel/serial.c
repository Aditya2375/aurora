#include "serial.h"
#include "ports.h"

/* COM1 debug console. The test harness asserts on this output. */

#define PORT 0x3F8

void serial_init(void) {
    outb(PORT + 1, 0x00);    /* interrupts off */
    outb(PORT + 3, 0x80);    /* DLAB on */
    outb(PORT + 0, 0x03);    /* 38400 baud */
    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x03);    /* 8N1 */
    outb(PORT + 2, 0xC7);    /* FIFO on, 14-byte threshold */
    outb(PORT + 4, 0x0B);    /* DTR, RTS, OUT2 */
}

void serial_putc(char c) {
    while ((inb(PORT + 5) & 0x20) == 0)
        ;
    outb(PORT, (uint8_t)c);
}

void serial_write(const char *s) {
    while (*s) {
        if (*s == '\n')
            serial_putc('\r');
        serial_putc(*s++);
    }
}
