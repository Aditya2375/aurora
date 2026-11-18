#ifndef AURORA_HEAP_H
#define AURORA_HEAP_H

#include <stddef.h>
#include <stdint.h>

void    heap_init(void);
void   *kmalloc(size_t size);
void    kfree(void *ptr);
size_t  heap_used(void);
size_t  heap_total(void);
int     heap_check(void);   /* 1 when the block list is internally consistent */

#endif
