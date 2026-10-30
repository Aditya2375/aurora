#ifndef AURORA_PMM_H
#define AURORA_PMM_H

#include <stddef.h>
#include <stdint.h>

#define PMM_BLOCK_SIZE 4096

void     pmm_init(void);
uint32_t pmm_alloc(void);
void     pmm_free(uint32_t addr);
uint32_t pmm_total_memory(void);   /* bytes */
uint32_t pmm_usable_blocks(void);
uint32_t pmm_free_blocks(void);

#endif

uint32_t pmm_alloc_contig(uint32_t npages);   /* base of a contiguous run, 0 on failure */
