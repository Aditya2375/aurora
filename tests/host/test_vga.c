#include "harness.h"
#include <stdint.h>
#include <string.h>

/* Run the real vga.c against a host buffer instead of 0xB8000. */
static uint16_t fake_mem[80 * 25];
#define VGA_MEM_ADDR ((uintptr_t)fake_mem)
#define VGA_NO_HW_CURSOR 1
#include "kernel/vga.c"

int main(void) {
    vga_clear();

    /* backspace in the middle of a row */
    vga_write("ab");
    vga_putc('\b');
    int row, col;
    vga_get_cursor(&row, &col);
    CHECK(row == 0 && col == 1, "backspace steps one cell");
    CHECK((fake_mem[1] & 0xFF) == ' ', "backspace blanks the cell");
    CHECK((fake_mem[0] & 0xFF) == 'a', "earlier cells untouched");

    /* fill a row exactly, wrap, then backspace: must step back up */
    vga_clear();
    for (int i = 0; i < 80; i++)
        vga_putc('x');
    vga_get_cursor(&row, &col);
    CHECK(row == 1 && col == 0, "full row wraps to next row");
    vga_putc('\b');
    vga_get_cursor(&row, &col);
    CHECK(row == 0 && col == 79, "backspace after wrap returns to row above");
    CHECK((fake_mem[79] & 0xFF) == ' ', "wrapped cell blanked");

    /* backspace at the very top-left is a no-op */
    vga_clear();
    vga_putc('\b');
    vga_get_cursor(&row, &col);
    CHECK(row == 0 && col == 0, "backspace at origin stays put");

    /* scroll still works */
    vga_clear();
    for (int r = 0; r < 25; r++)
        vga_write("0123456789\n");
    vga_get_cursor(&row, &col);
    CHECK(row == 24 && col == 0, "screen scrolls at the bottom");
    CHECK((fake_mem[0] & 0xFF) == '0', "content scrolled up");

    TEST_RESULTS();
}
