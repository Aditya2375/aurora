#include "harness.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Capture what kprintf emits by standing in for the VGA and serial drivers. */
static char vga_log[8192];
static char serial_log[8192];

static void capture(char *dst, const char *s) {
    size_t used = strlen(dst);
    size_t room = 8192 - used - 1;
    size_t n = strlen(s);
    if (n > room)
        n = room;
    memcpy(dst + used, s, n);
    dst[used + n] = 0;
}

uint8_t vga_get_attr(void) { return 7; }
void vga_set_attr(uint8_t a) { (void)a; }
void vga_write(const char *s) { capture(vga_log, s); }
void serial_write(const char *s) { capture(serial_log, s); }

#include "kernel/kprint.c"

static void reset_logs(void) { vga_log[0] = 0; serial_log[0] = 0; }

int main(void) {
    reset_logs();
    kprintf("hello %s", "world");
    CHECK(strcmp(vga_log, "hello world") == 0, "string substitution");
    CHECK(strcmp(serial_log, "hello world") == 0, "serial mirrors vga");

    reset_logs();
    kprintf("%d %d %d", 0, 42, -7);
    CHECK(strcmp(vga_log, "0 42 -7") == 0, "signed decimals");

    reset_logs();
    kprintf("%u", 4294967295u);
    CHECK(strcmp(vga_log, "4294967295") == 0, "unsigned max");

    reset_logs();
    kprintf("%x %x", 0xDEADBEEFu, 0u);
    CHECK(strcmp(vga_log, "deadbeef 0") == 0, "hex");

    reset_logs();
    kprintf("100%%");
    CHECK(strcmp(vga_log, "100%") == 0, "percent escape");

    reset_logs();
    kprintf("%c%c%c", 'O', 'S', '!');
    CHECK(strcmp(vga_log, "OS!") == 0, "chars");

    reset_logs();
    { const char *volatile nothing = 0; kprintf("%s", nothing); }
    CHECK(strcmp(vga_log, "(null)") == 0, "null string guarded");

    reset_logs();
    kprintf("%lld", -9223372036854775807ll - 1);   /* LLONG_MIN, no negation UB */
    CHECK(strcmp(vga_log, "-9223372036854775808") == 0, "LLONG_MIN no overflow");

    reset_logs();
    kprintf("%d", -2147483647 - 1);                 /* INT_MIN through the int path */
    CHECK(strcmp(vga_log, "-2147483648") == 0, "INT_MIN no overflow");

    reset_logs();
    {   /* unknown specifier: non-literal format, passed through untouched */
        char dynfmt[] = "tail%z";
        kprintf(dynfmt, 0);   /* extra arg ignored */
        CHECK(strcmp(vga_log, "tail%z") == 0, "unknown specifier passed through");
    }

    reset_logs();
    kprintf_attr(0x0C, "attr %d", 5);
    CHECK(strcmp(vga_log, "attr 5") == 0, "attr variant formats");

    reset_logs();
    kprintf("[%02x] [%04x]", 0xau, 0x123u);
    CHECK(strcmp(vga_log, "[0a] [0123]") == 0, "zero-padded hex widths");

    reset_logs();
    kprintf("[%02x]", 0x123u);
    CHECK(strcmp(vga_log, "[123]") == 0, "width is a minimum, not a cap");

    reset_logs();
    kprintf("[%5d] [%-s]", 42, "x");
    CHECK(strcmp(vga_log, "[   42] [%-s]") == 0, "space-padded width; no left-align flag");

    reset_logs();
    kprintf("[%05d]", -42);
    CHECK(strcmp(vga_log, "[-0042]") == 0, "zero pad after the sign");

    reset_logs();
    kprintf("[%08llx]", 0xabcdefull);
    CHECK(strcmp(vga_log, "[00abcdef]") == 0, "zero-padded 64-bit hex");

    reset_logs();
    kprintf("[%6s]", "hi");
    CHECK(strcmp(vga_log, "[    hi]") == 0, "space-padded string width");

    reset_logs();
    /* a wide conversion at the end of a nearly-full line must truncate
       against the 512-byte buffer, never overflow it */
    {   char big[600];
        memset(big, 'a', sizeof big - 1);
        big[sizeof big - 1] = 0;
        kprintf("%s%d", big, 42);
        CHECK(strlen(vga_log) == 511, "long line truncated to the buffer");
        CHECK(vga_log[510] == 'a', "truncation is clean, digits dropped");
    }

    reset_logs();
    {   char big[600];
        memset(big, 'b', 500);
        big[500] = 0;
        kprintf("%s%064llu", big, 42ull);   /* pad loop would overflow by ~50 */
        CHECK(strlen(vga_log) == 511, "wide padded number truncated safely");
    }

    TEST_RESULTS();
}
