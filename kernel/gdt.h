#ifndef AURORA_GDT_H
#define AURORA_GDT_H

#include <stdint.h>

#define KERNEL_CS 0x08
#define KERNEL_DS 0x10

void gdt_init(void);

#endif
