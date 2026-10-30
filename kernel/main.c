#include "gdt.h"
#include "heap.h"
#include "idt.h"
#include "kbd.h"
#include "kprint.h"
#include "pmm.h"
#include "serial.h"
#include "shell.h"
#include "string.h"
#include "timer.h"
#include "vga.h"

#define AURORA_VERSION "0.1"

static void banner(void) {
    kprintf_attr(ATTR_TITLE,
        "    ___\n"
        "   /   |  __  ___________  ____ _\n"
        "  / /| | / / / / ___/ __ \\/ __ `/\n"
        " / ___ |/ /_/ / /  / /_/ / /_/ /\n"
        "/_/  |_|\\__,_/_/   \\____/\\__,_/\n");
    kprintf_attr(ATTR_DIM, "a from-scratch x86 operating system - v%s\n\n", AURORA_VERSION);
}

/* Boot-time smoke tests, always on. Failures are loud on screen and serial. */
static int boot_tests(void) {
    int failed = 0;

    char a[32], b[32];
    memset(a, 'x', sizeof(a));
    memcpy(b, a, sizeof(b));
    if (memcmp(a, b, sizeof(a)) != 0) { serial_write("BOOTTEST string FAIL\n"); failed++; }

    uint32_t f0 = pmm_free_blocks();
    uint32_t p1 = pmm_alloc();
    uint32_t p2 = pmm_alloc();
    if (!p1 || !p2 || p1 == p2) { serial_write("BOOTTEST pmm FAIL\n"); failed++; }
    pmm_free(p1);
    pmm_free(p2);
    if (pmm_free_blocks() != f0) { serial_write("BOOTTEST pmm leak FAIL\n"); failed++; }

    void *x = kmalloc(24);
    void *y = kmalloc(5000);
    if (!x || !y || !heap_check()) { serial_write("BOOTTEST heap FAIL\n"); failed++; }
    kfree(x);
    kfree(y);
    if (!heap_check()) { serial_write("BOOTTEST heap coalesce FAIL\n"); failed++; }

    if (!failed)
        serial_write("BOOTTEST all PASS\n");
    return failed;
}

void kmain(void) {
    serial_init();
    serial_write("\nAURORA BOOT SERIAL\n");

    vga_init();
    gdt_init();
    idt_init();
    pmm_init();
    heap_init();
    timer_init();
    kbd_init();
    __asm__ volatile ("sti");

    if (boot_tests() != 0) {
        kprintf_attr(ATTR_ERR, "boot self-tests FAILED - halting before shell\n");
        for (;;)
            __asm__ volatile ("cli; hlt");
    }
    serial_write("KERNEL READY\n");

    banner();
    kprintf("detected memory: %u MiB, free pages: %u\n",
            (unsigned long long)(pmm_total_memory() / (1024 * 1024)),
            (unsigned long long)pmm_free_blocks());
    kprintf_attr(ATTR_OK, "boot self-tests passed. ");
    kprintf("type 'help' for commands.\n\n");

    shell_run();
}
