#include "kbd.h"
#include "idt.h"
#include "ports.h"
#include "serial.h"

#define BUF_SIZE 128

/* PS/2 set 1 scancodes. Non-US layouts are out of scope. */
static const char map_lower[128] = {
    0,   27,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'', '`', 0,  '\\',
    'z','x','c','v','b','n','m',',','.','/', 0,   '*', 0,   ' ',
    0,   0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,   0,   0,  0,  0,  0,  0,  '7','8','9','-','4','5','6','+',
    '1','2','3','0','.', 0,  0,  0,  0,  0,
};

static const char map_upper[128] = {
    0,   27,  '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"', '~', 0,  '|',
    'Z','X','C','V','B','N','M','<','>','?', 0,   '*', 0,   ' ',
    0,   0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,   0,   0,  0,  0,  0,  0,  '7','8','9','-','4','5','6','+',
    '1','2','3','0','.', 0,  0,  0,  0,  0,
};

static volatile char buf[BUF_SIZE];
static volatile int head = 0, tail = 0;
static bool shift = false;
static bool caps = false;

static void buf_push(char c) {
    int next = (head + 1) % BUF_SIZE;
    if (next == tail)
        return;             /* full: drop the key rather than corrupt */
    buf[head] = c;
    head = next;
}

static void kbd_callback(struct registers *regs) {
    (void)regs;
    uint8_t sc = inb(0x60);

    if (sc == 0xE0) {       /* extended prefix: read and ignore the next byte */
        while ((inb(0x64) & 1) == 0)
            ;
        (void)inb(0x60);
        return;
    }
    bool released = (sc & 0x80) != 0;
    sc &= 0x7F;

    if (sc == 0x2A || sc == 0x36) {
        shift = !released;
        return;
    }
    if (sc == 0x3A) {
        if (!released)
            caps = !caps;
        return;
    }
    if (released)
        return;

    char c = shift ? map_upper[sc] : map_lower[sc];
    if (c >= 'a' && c <= 'z' && caps)
        c = (char)(shift ? c + 32 : c - 32);
    if (c)
        buf_push(c);
}

void kbd_init(void) {
    while (inb(0x64) & 1)   /* flush stale controller output */
        (void)inb(0x60);
    register_interrupt_handler(IRQ1, kbd_callback);
    irq_enable(1);
}

bool kbd_poll(char *c) {
    if (head == tail)
        return false;
    *c = buf[tail];
    tail = (tail + 1) % BUF_SIZE;
    return true;
}

char kbd_getc(void) {
    char c;
    while (!kbd_poll(&c))
        __asm__ volatile ("hlt");
    return c;
}
