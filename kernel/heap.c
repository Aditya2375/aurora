#include "heap.h"
#include "pmm.h"
#include "string.h"

/* Free-list heap over pages claimed from the PMM.
 * Blocks are 16-byte aligned; header sits in front of every allocation. */

#define ALIGN_UP(n) (((n) + 15u) & ~15u)
#define MIN_SPLIT   32u

/* The kernel runs on an identity map, so physical addresses are used as
 * pointers directly. Host unit tests substitute an arena-backed mapping. */
#ifndef AURORA_PHYS_TO_VIRT
#define AURORA_PHYS_TO_VIRT(addr) ((void *)(uintptr_t)(addr))
#endif

struct block {
    uint32_t size;          /* payload bytes */
    int      free;
    struct block *prev;
    struct block *next;
};

static struct block *first_block = 0;
static struct block *last_block = 0;
static size_t used_bytes = 0;
static size_t total_bytes = 0;

static int heap_expand(size_t need) {
    size_t span = need + sizeof(struct block);
    size_t pages = (span + PMM_BLOCK_SIZE - 1) / PMM_BLOCK_SIZE;

    uint32_t first = pmm_alloc_contig((uint32_t)pages);
    if (!first)
        return 0;

    struct block *b = (struct block *)AURORA_PHYS_TO_VIRT(first);
    b->size = (uint32_t)(pages * PMM_BLOCK_SIZE - sizeof(struct block));
    b->free = 1;
    b->prev = last_block;
    b->next = 0;
    if (last_block)
        last_block->next = b;
    else
        first_block = b;
    last_block = b;
    total_bytes += b->size;
    return 1;
}

static void split(struct block *b, size_t size) {
    if (b->size < size + sizeof(struct block) + MIN_SPLIT)
        return;
    struct block *rest = (struct block *)((uint8_t *)(b + 1) + size);
    rest->size = (uint32_t)(b->size - size - sizeof(struct block));
    rest->free = 1;
    rest->prev = b;
    rest->next = b->next;
    if (rest->next)
        rest->next->prev = rest;
    else
        last_block = rest;
    b->next = rest;
    b->size = (uint32_t)size;
}

static void coalesce(struct block *b) {
    if (b->next && b->next->free) {
        struct block *n = b->next;
        b->size += n->size + sizeof(struct block);
        b->next = n->next;
        if (n->next)
            n->next->prev = b;
        else
            last_block = b;
    }
    if (b->prev && b->prev->free) {
        struct block *p = b->prev;
        p->size += b->size + sizeof(struct block);
        p->next = b->next;
        if (b->next)
            b->next->prev = p;
        else
            last_block = p;
    }
}

void heap_init(void) {
    first_block = 0;
    last_block = 0;
    used_bytes = 0;
    total_bytes = 0;
}

void *kmalloc(size_t size) {
    if (size == 0)
        return 0;
    size = ALIGN_UP(size);

    struct block *best = 0;
    for (struct block *b = first_block; b; b = b->next) {
        if (b->free && b->size >= size && (!best || b->size < best->size))
            best = b;
    }
    if (!best) {
        if (!heap_expand(size))
            return 0;
        best = last_block;
    }
    split(best, size);
    best->free = 0;
    used_bytes += best->size;
    return best + 1;
}

void kfree(void *ptr) {
    if (!ptr)
        return;
    struct block *b = ((struct block *)ptr) - 1;
    if (b->free)
        return;                     /* double free guard */
    b->free = 1;
    used_bytes -= b->size;
    coalesce(b);
}

size_t heap_used(void)  { return used_bytes; }
size_t heap_total(void) { return total_bytes; }

int heap_check(void) {
    struct block *prev = 0;
    for (struct block *b = first_block; b; b = b->next) {
        if (b->prev != prev)
            return 0;
        if (b->size == 0 || b->size > total_bytes + PMM_BLOCK_SIZE)
            return 0;
        prev = b;
    }
    return prev == last_block;
}
