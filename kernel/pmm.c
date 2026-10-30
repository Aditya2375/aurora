#include "pmm.h"
#include "string.h"

/* Physical page allocator: one bitmap bit per 4 KiB frame. */

/* Absolute addresses are overridable so the host test suite can inject maps. */
#ifndef E820_COUNT_ADDR
#define E820_COUNT_ADDR 0x00004FF0u
#endif
#ifndef E820_TABLE_ADDR
#define E820_TABLE_ADDR 0x00005000u
#endif
#define MAX_E820        64
#define BITMAP_LIMIT    (512u * 1024u * 1024u)          /* manage the low 512 MiB */
#define MAX_BLOCKS      (BITMAP_LIMIT / PMM_BLOCK_SIZE)  /* 131072 */
#define BITMAP_BYTES    (MAX_BLOCKS / 8)                 /* 16 KiB */

#ifndef AURORA_E820_ENTRY_DEFINED
#define AURORA_E820_ENTRY_DEFINED
struct e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed));
#endif

#ifndef AURORA_KERNEL_END
extern uint8_t _bss_end;
#define AURORA_KERNEL_END ((uint32_t)&_bss_end)
#endif

static uint8_t bitmap[BITMAP_BYTES];
static uint32_t n_usable_blocks = 0;
static uint32_t n_free_blocks = 0;
static uint32_t total_memory = 0;

static inline void bit_set(uint32_t i)   { bitmap[i / 8] |=  (uint8_t)(1u << (i % 8)); }
static inline void bit_clear(uint32_t i) { bitmap[i / 8] &= (uint8_t)~(1u << (i % 8)); }
static inline int  bit_test(uint32_t i)  { return (bitmap[i / 8] >> (i % 8)) & 1; }

void pmm_init(void) {
    uint16_t count = *(volatile uint16_t *)E820_COUNT_ADDR;
    if (count > MAX_E820)
        count = MAX_E820;
    const volatile struct e820_entry *map = (volatile struct e820_entry *)E820_TABLE_ADDR;

    /* Everything starts reserved; free only regions the firmware says are usable. */
    memset(bitmap, 0xFF, sizeof(bitmap));

    for (uint32_t i = 0; i < count; i++) {
        uint64_t end = map[i].base + map[i].length;
        if (map[i].type != 1)
            continue;
        if (end > total_memory)
            total_memory = (uint32_t)(end > 0xFFFFFFFFull ? 0xFFFFFFFFull : end);
        uint64_t first = (map[i].base + PMM_BLOCK_SIZE - 1) & ~(uint64_t)(PMM_BLOCK_SIZE - 1);
        uint64_t last  = end & ~(uint64_t)(PMM_BLOCK_SIZE - 1);
        for (uint64_t a = first; a < last && a < BITMAP_LIMIT; a += PMM_BLOCK_SIZE) {
            uint32_t b = (uint32_t)(a / PMM_BLOCK_SIZE);
            if (bit_test(b)) {
                bit_clear(b);
                n_usable_blocks++;
            }
        }
    }

    /* Never hand out the low 1 MiB: IVT, BIOS data, EBDA, VGA. */
    for (uint32_t b = 0; b < 256; b++) {
        if (!bit_test(b)) {
            bit_set(b);
            n_usable_blocks--;
        }
    }

    /* Reserve the kernel image, the page directory the loader left at 0x50000,
     * and the kernel stack at 0x1F0000-0x200000 (see kernel/entry.asm). */
    uint32_t kernel_end = AURORA_KERNEL_END;
    for (uint32_t a = 0x50000; a < kernel_end && a < BITMAP_LIMIT; a += PMM_BLOCK_SIZE) {
        uint32_t b = a / PMM_BLOCK_SIZE;
        if (b < MAX_BLOCKS && !bit_test(b)) {
            bit_set(b);
            n_usable_blocks--;
        }
    }
    for (uint32_t a = 0x1F0000; a < 0x200000; a += PMM_BLOCK_SIZE) {
        uint32_t b = a / PMM_BLOCK_SIZE;
        if (b < MAX_BLOCKS && !bit_test(b)) {
            bit_set(b);
            n_usable_blocks--;
        }
    }

    n_free_blocks = n_usable_blocks;
}

uint32_t pmm_alloc(void) {
    for (uint32_t b = 0; b < MAX_BLOCKS; b++) {
        if (!bit_test(b)) {
            bit_set(b);
            n_free_blocks--;
            return b * PMM_BLOCK_SIZE;
        }
    }
    return 0;               /* out of memory */
}

void pmm_free(uint32_t addr) {
    if (addr == 0 || addr >= BITMAP_LIMIT)
        return;
    uint32_t b = addr / PMM_BLOCK_SIZE;
    if (bit_test(b)) {
        bit_clear(b);
        n_free_blocks++;
    }
}

uint32_t pmm_total_memory(void)  { return total_memory; }
uint32_t pmm_usable_blocks(void) { return n_usable_blocks; }
uint32_t pmm_free_blocks(void)   { return n_free_blocks; }

uint32_t pmm_alloc_contig(uint32_t npages) {
    if (npages == 0)
        return 0;
    uint32_t run = 0;
    for (uint32_t b = 0; b < MAX_BLOCKS; b++) {
        if (bit_test(b)) {
            run = 0;
            continue;
        }
        run++;
        if (run == npages) {
            uint32_t base = b - npages + 1;
            for (uint32_t i = base; i <= b; i++)
                bit_set(i);
            n_free_blocks -= npages;
            return base * PMM_BLOCK_SIZE;
        }
    }
    return 0;
}
