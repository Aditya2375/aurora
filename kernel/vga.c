#include "vga.h"
#include "string.h"
#include "ports.h"

#ifndef VGA_MEM_ADDR
#define VGA_MEM_ADDR 0x000B8000
#endif
#define VGA_MEM   ((volatile uint16_t *)VGA_MEM_ADDR)
#define VGA_COLS  80
#define VGA_ROWS  25

static uint8_t cur_attr = 0x07;
static int cur_row = 0;
static int cur_col = 0;

#ifndef VGA_NO_HW_CURSOR
static void hw_cursor_update(void) {
    uint16_t pos = (uint16_t)(cur_row * VGA_COLS + cur_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(pos >> 8));
}
#else
static void hw_cursor_update(void) { }
#endif

void vga_set_attr(uint8_t attr) { cur_attr = attr; }
uint8_t vga_get_attr(void) { return cur_attr; }
void vga_get_cursor(int *row, int *col) { *row = cur_row; *col = cur_col; }

static void scroll(void) {
    memmove((void *)VGA_MEM, (const void *)(VGA_MEM + VGA_COLS),
            (VGA_ROWS - 1) * VGA_COLS * 2);
    for (int c = 0; c < VGA_COLS; c++)
        VGA_MEM[(VGA_ROWS - 1) * VGA_COLS + c] = ((uint16_t)cur_attr << 8) | ' ';
}

void vga_init(void) {
    cur_row = 0;
    cur_col = 0;
    vga_clear();
}

void vga_clear(void) {
    for (int i = 0; i < VGA_ROWS * VGA_COLS; i++)
        VGA_MEM[i] = ((uint16_t)cur_attr << 8) | ' ';
    cur_row = 0;
    cur_col = 0;
    hw_cursor_update();
}

void vga_putc(char c) {
    if (c == '\n') {
        cur_col = 0;
        cur_row++;
    } else if (c == '\t') {
        cur_col = (cur_col + 8) & ~7;
    } else if (c == '\b') {
        if (cur_col > 0)
            cur_col--;
        else if (cur_row > 0) {         /* wrapped line: step back up a row */
            cur_row--;
            cur_col = VGA_COLS - 1;
        }
        VGA_MEM[cur_row * VGA_COLS + cur_col] = ((uint16_t)cur_attr << 8) | ' ';
    } else {
        VGA_MEM[cur_row * VGA_COLS + cur_col] = ((uint16_t)cur_attr << 8) | (uint8_t)c;
        cur_col++;
    }
    if (cur_col >= VGA_COLS) {
        cur_col = 0;
        cur_row++;
    }
    while (cur_row >= VGA_ROWS) {
        scroll();
        cur_row--;
    }
#ifndef VGA_NO_HW_CURSOR
    hw_cursor_update();
#endif
}

void vga_write(const char *s) {
    while (*s)
        vga_putc(*s++);
}
