#ifndef AURORA_MEMORY_H
#define AURORA_MEMORY_H
#include <stdint.h>
#include "idt.h"
void memory_init(void);
int memory_check(void);
int memory_handle_fault(struct registers *r);
uint32_t memory_faults_caught(void);
#endif
