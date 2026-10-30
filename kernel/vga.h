#ifndef AURORA_VGA_H
#define AURORA_VGA_H

#include <stdint.h>

enum vga_color {
    VGA_BLACK = 0, VGA_BLUE, VGA_GREEN, VGA_CYAN,
    VGA_RED, VGA_MAGENTA, VGA_BROWN, VGA_LIGHT_GRAY,
    VGA_DARK_GRAY, VGA_LIGHT_BLUE, VGA_LIGHT_GREEN, VGA_LIGHT_CYAN,
    VGA_LIGHT_RED, VGA_LIGHT_MAGENTA, VGA_YELLOW, VGA_WHITE,
};

#define VGA_ATTR(fg, bg) ((uint8_t)((fg) | ((bg) << 4)))

void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_write(const char *s);
void vga_set_attr(uint8_t attr);
uint8_t vga_get_attr(void);
void vga_get_cursor(int *row, int *col);

#endif
