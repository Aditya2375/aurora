#include "harness.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Fake E820 map: 64 MiB usable RAM at 1 MiB, plus a reserved hole at 32 MiB. */

#define AURORA_E820_ENTRY_DEFINED
struct e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed));

static uint16_t fake_count;
static struct e820_entry fake_map[8];
#define AURORA_KERNEL_END 0x150000u   /* pretend the kernel image ends at 1.25 MiB */

#define E820_COUNT_ADDR (&fake_count)
#define E820_TABLE_ADDR (fake_map)

#include "../../kernel/pmm.c"

/* Back the heap's physical addresses with a real host arena. */
static uint8_t *arena;
#define AURORA_PHYS_TO_VIRT(addr) (arena + ((uint32_t)(addr) - 0x100000u))
#include "../../kernel/heap.c"


static void setup(void) {
    fake_count = 3;
    fake_map[0] = (struct e820_entry){0x00000000, 0x0009FC00, 1, 1};  /* low RAM */
    fake_map[1] = (struct e820_entry){0x00100000, 64u * 1024 * 1024, 1, 1};
    fake_map[2] = (struct e820_entry){0x02000000, 0x00100000, 2, 1};   /* reserved */
    arena = malloc(80u * 1024 * 1024);
    if (!arena) { printf("arena alloc failed\n"); exit(1); }
    pmm_init();
    heap_init();
}

int main(void) {
    setup();

    CHECK(pmm_total_memory() == 65u * 1024 * 1024, "total memory from usable entries");

    uint32_t f0 = pmm_free_blocks();
    CHECK(f0 > 15000 && f0 < 17000, "usable block count sane for 64+1 MiB");

    uint32_t a = pmm_alloc();
    uint32_t b = pmm_alloc();
    CHECK(a != 0 && b != 0 && a != b, "allocations are distinct");
    CHECK(a % PMM_BLOCK_SIZE == 0, "allocations are page aligned");
    CHECK(pmm_free_blocks() == f0 - 2, "free count drops");
    CHECK(a < 0x02000000 || a >= 0x02100000, "reserved hole never allocated");
    pmm_free(a);
    pmm_free(b);
    CHECK(pmm_free_blocks() == f0, "free count restored");
    pmm_free(a);                                   /* double free */
    CHECK(pmm_free_blocks() == f0, "double free is a no-op");
    pmm_free(0xDEAD0000);                          /* out of managed range */
    CHECK(pmm_free_blocks() == f0, "wild free rejected");

    uint32_t run = pmm_alloc_contig(300);
    CHECK(run != 0, "contiguous 300-page run exists");
    uint32_t inside = pmm_alloc();
    CHECK(inside < run || inside >= run + 300 * PMM_BLOCK_SIZE, "run stays intact");
    for (int i = 0; i < 300; i++)
        pmm_free(run + i * PMM_BLOCK_SIZE);
    pmm_free(inside);

    /* heap: basic alloc/free */
    void *p = kmalloc(24);
    CHECK(p != NULL, "kmalloc 24");
    /* Kernel builds are 32-bit: 16-byte block header on a page base gives
     * 16-byte payloads. This 64-bit host build has a 24-byte header, so the
     * same construction yields 8-byte payloads here. */
    const uintptr_t expect = sizeof(void *) == 8 ? 7 : 15;
    CHECK(((uintptr_t)p & expect) == 0, "kmalloc aligned to header boundary");
    memset(p, 0x5A, 24);
    CHECK(heap_used() >= 24, "heap tracks usage");
    kfree(p);
    CHECK(heap_used() == 0, "heap usage returns to zero");
    CHECK(heap_check(), "heap consistent after small cycle");

    /* heap: larger than one page */
    void *big = kmalloc(10000);
    CHECK(big != NULL, "kmalloc 10000 (multi-page)");
    memset(big, 0xA5, 10000);
    kfree(big);
    CHECK(heap_check(), "heap consistent after multi-page cycle");

    /* heap: many mixed sizes, then free in reverse */
    void *ptrs[64];
    for (int i = 0; i < 64; i++) {
        ptrs[i] = kmalloc((size_t)(i * 37 + 1));
        CHECK(ptrs[i] != NULL, "mixed alloc");
        memset(ptrs[i], i & 0xFF, (size_t)(i * 37 + 1));
    }
    for (int i = 63; i >= 0; i--)
        kfree(ptrs[i]);
    CHECK(heap_used() == 0, "all mixed allocs freed");
    CHECK(heap_check(), "heap consistent after storm");

    /* heap: free order shuffle forces coalescing both ways */
    void *x = kmalloc(100), *y = kmalloc(100), *z = kmalloc(100);
    kfree(y);
    kfree(z);
    kfree(x);
    void *w = kmalloc(280);     /* must fit in the coalesced space */
    CHECK(w == x, "coalesced block reused");
    kfree(w);
    CHECK(heap_check(), "heap consistent after coalesce");

    /* heap: exhaustion fails cleanly, double free guarded */
    void *huge = kmalloc(512u * 1024 * 1024);
    CHECK(huge == NULL, "over-limit kmalloc fails cleanly");
    CHECK(heap_check(), "heap consistent after failed alloc");
    void *d = kmalloc(8);
    kfree(d);
    size_t before = heap_used();
    kfree(d);                                    /* double free */
    CHECK(heap_used() == before, "double kfree guarded");
    CHECK(heap_check(), "heap consistent after double free");

    TEST_RESULTS();
}
