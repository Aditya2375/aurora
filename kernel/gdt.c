#include "gdt.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

extern void gdt_flush(uint32_t ptr);

static struct gdt_entry gdt[3];
static struct gdt_ptr gp;

static void gdt_set(int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[i].base_low  = (uint16_t)(base & 0xFFFF);
    gdt[i].base_mid  = (uint8_t)((base >> 16) & 0xFF);
    gdt[i].base_high = (uint8_t)((base >> 24) & 0xFF);
    gdt[i].limit_low = (uint16_t)(limit & 0xFFFF);
    gdt[i].gran      = (uint8_t)(((limit >> 16) & 0x0F) | (gran & 0xF0));
    gdt[i].access    = access;
}

void gdt_init(void) {
    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint32_t)&gdt;

    gdt_set(0, 0, 0, 0, 0);
    gdt_set(1, 0, 0xFFFFF, 0x9A, 0xCF);   /* kernel code */
    gdt_set(2, 0, 0xFFFFF, 0x92, 0xCF);   /* kernel data */

    gdt_flush((uint32_t)&gp);
}
