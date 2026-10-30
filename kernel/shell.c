#include "shell.h"
#include "kbd.h"
#include "kprint.h"
#include "vga.h"
#include "heap.h"
#include "pmm.h"
#include "serial.h"
#include "string.h"
#include "timer.h"

#define LINE_MAX 128
#define HIST_MAX 16

static char history[HIST_MAX][LINE_MAX];
static int hist_len = 0;

static void hist_push(const char *line) {
    if (hist_len == HIST_MAX) {
        for (int i = 1; i < HIST_MAX; i++)
            strcpy(history[i - 1], history[i]);
        hist_len--;
    }
    strcpy(history[hist_len++], line);
}

static void prompt(void) {
    kprintf_attr(ATTR_ACCENT, "aurora");
    kprintf_attr(ATTR_DIM, ":");
    kprintf_attr(ATTR_TITLE, "~");
    kprintf_attr(ATTR_DIM, "$ ");
}

static void read_line(char *line) {
    int len = 0;
    for (;;) {
        char c = kbd_getc();
        if (c == '\n') {
            kprintf("\n");
            line[len] = '\0';
            return;
        }
        if (c == '\b') {
            if (len > 0) {
                len--;
                kprintf("\b");
            }
            continue;
        }
        if (len < LINE_MAX - 1) {
            line[len++] = c;
            kprintf("%c", (int)c);
        }
    }
}

static void cmd_help(void) {
    kprintf_attr(ATTR_TITLE, "commands:\n");
    kprintf("  help       this list\n");
    kprintf("  about      what Aurora is\n");
    kprintf("  mem        physical memory and heap usage\n");
    kprintf("  alloc N    allocate N bytes, then free them (heap demo)\n");
    kprintf("  uptime     time since boot\n");
    kprintf("  echo TEXT  print text\n");
    kprintf("  history    recent commands\n");
    kprintf("  color      VGA color test card\n");
    kprintf("  clear      clear the screen\n");
    kprintf("  halt       stop the CPU\n");
}

static void cmd_about(void) {
    kprintf_attr(ATTR_TITLE, "Aurora OS v0.1\n");
    kprintf("A 32-bit x86 operating system written from scratch.\n");
    kprintf("Two-stage bootloader, protected mode, identity paging, E820 memory\n");
    kprintf("map, bitmap physical allocator, free-list heap, interrupt-driven\n");
    kprintf("timer and keyboard, and this shell. No bootloader framework, no\n");
    kprintf("standard library - everything on screen came from this tree.\n");
}

static void cmd_mem(void) {
    kprintf("e820 total memory : %u MiB\n", (unsigned)(pmm_total_memory() / (1024 * 1024)));
    kprintf("pmm free pages    : %u of %u\n",
            (unsigned)pmm_free_blocks(), (unsigned)pmm_usable_blocks());
    kprintf("heap              : %u bytes used, %u bytes mapped\n",
            (unsigned)heap_used(), (unsigned)heap_total());
    kprintf("heap integrity    : %s\n", heap_check() ? "ok" : "BROKEN");
}

static void cmd_uptime(void) {
    uint64_t t = timer_ticks();
    uint64_t cs = t % TIMER_HZ;
    uint64_t s = (t / TIMER_HZ) % 60;
    uint64_t m = (t / (TIMER_HZ * 60)) % 60;
    uint64_t h = t / (TIMER_HZ * 3600);
    kprintf("up %llu:%llu%llu:%llu%llu.%llu%llu\n",
            (unsigned long long)h,
            (unsigned long long)(m / 10), (unsigned long long)(m % 10),
            (unsigned long long)(s / 10), (unsigned long long)(s % 10),
            (unsigned long long)(cs / 10), (unsigned long long)(cs % 10));
}

static void cmd_color(void) {
    for (int bg = 0; bg < 8; bg++) {
        for (int fg = 0; fg < 16; fg++)
            kprintf_attr(VGA_ATTR(fg, bg), "%c", (int)0xDB);
        kprintf("\n");
    }
}

static void cmd_alloc(const char *arg) {
    if (!*arg) {
        kprintf("usage: alloc <bytes>\n");
        return;
    }
    long long n = 0;
    for (const char *p = arg; *p; p++) {
        if (*p < '0' || *p > '9') {
            kprintf("usage: alloc <bytes>\n");
            return;
        }
        n = n * 10 + (*p - '0');
        if (n > 64 * 1024 * 1024)
            break;
    }
    void *p = kmalloc((size_t)n);
    if (!p) {
        kprintf_attr(ATTR_ERR, "kmalloc(%u) failed\n", (unsigned)n);
        return;
    }
    memset(p, 0x5A, (size_t)n);     /* touch every page */
    kprintf("kmalloc(%u) -> 0x%x, touched, heap check %s, freeing\n",
            (unsigned)n, (unsigned)(uint32_t)p,
            heap_check() ? "ok" : "BROKEN");
    kfree(p);
    kprintf("freed, heap check %s, heap used %u bytes\n",
            heap_check() ? "ok" : "BROKEN", (unsigned)heap_used());
}

static void execute(char *line) {
    while (*line == ' ')
        line++;
    if (!*line)
        return;
    hist_push(line);

    char *arg = line;
    while (*arg && *arg != ' ')
        arg++;
    if (*arg) {
        *arg = '\0';
        arg++;
        while (*arg == ' ')
            arg++;
    }

    if (strcmp(line, "help") == 0)          cmd_help();
    else if (strcmp(line, "about") == 0)    cmd_about();
    else if (strcmp(line, "mem") == 0)      cmd_mem();
    else if (strcmp(line, "uptime") == 0)   cmd_uptime();
    else if (strcmp(line, "color") == 0)    cmd_color();
    else if (strcmp(line, "alloc") == 0)    cmd_alloc(arg);
    else if (strcmp(line, "echo") == 0)     kprintf("%s\n", arg);
    else if (strcmp(line, "clear") == 0)    vga_clear();
    else if (strcmp(line, "history") == 0) {
        for (int i = 0; i < hist_len; i++)
            kprintf("  %d  %s\n", i + 1, history[i]);
    }
    else if (strcmp(line, "halt") == 0) {
        kprintf_attr(ATTR_WARN, "halting\n");
        serial_write("GUEST_HALTED\n");
        __asm__ volatile ("cli; hlt");
    }
    else
        kprintf("unknown command '%s' - try 'help'\n", line);
}

void shell_run(void) {
    char line[LINE_MAX];
    for (;;) {
        prompt();
        read_line(line);
        execute(line);
    }
}
