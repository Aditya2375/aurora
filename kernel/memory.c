#include "memory.h"
#include "string.h"

/* A single supervisor address space. No user processes, and no NX in non-PAE. */
static uint32_t directory[1024] __attribute__((aligned(4096)));
static uint32_t tables[32][1024] __attribute__((aligned(4096)));
extern uint8_t _text_start, _ro_end;
extern void probe_read(uint32_t addr);
extern void probe_write(uint32_t addr);
extern uint8_t probe_read_fault, probe_read_resume;
extern uint8_t probe_write_fault, probe_write_resume;
static volatile uint32_t expected_pc, expected_addr, expected_error, resume_pc;
static volatile uint32_t caught;

void memory_init(void) {
    memset(directory, 0, sizeof(directory));
    for (uint32_t d = 0; d < 32; d++) {
        directory[d] = ((uint32_t)tables[d]) | 3u;
        for (uint32_t p = 0; p < 1024; p++) {
            uint32_t a = (d * 1024 + p) * 4096;
            uint32_t flags = 3;
            if (a >= (uint32_t)&_text_start && a < (uint32_t)&_ro_end)
                flags = 1; /* read-only, including in ring 0 once WP is set */
            if (a == 0 || a == 0x1EF000u)
                flags = 0; /* null and the page below the kernel stack */
            tables[d][p] = a | flags;
        }
    }
    __asm__ volatile ("mov %0, %%cr3" : : "r"((uint32_t)directory) : "memory");
    uint32_t cr0;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80010000u; /* paging + supervisor write-protect, even without PSE */
    __asm__ volatile ("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

/* Only these exact three diagnostic operations may recover a fault. A fault
 * anywhere else still reaches the fatal exception handler. */
int memory_handle_fault(struct registers *r) {
    uint32_t addr;
    __asm__ volatile ("mov %%cr2, %0" : "=r"(addr));
    if (!expected_pc || r->eip != expected_pc || addr != expected_addr ||
        (r->err_code & 31u) != expected_error)
        return 0;
    expected_pc = 0;
    caught++;
    r->eip = resume_pc;
    return 1;
}

static int probe(uint32_t addr, int write) {
    uint32_t before = caught;
    expected_addr = addr;
    expected_error = write ? 3 : 0;
    resume_pc = (uint32_t)(write ? &probe_write_resume : &probe_read_resume);
    expected_pc = (uint32_t)(write ? &probe_write_fault : &probe_read_fault);
    if (write) probe_write(addr); else probe_read(addr);
    expected_pc = 0;
    return caught == before + 1;
}

int memory_check(void) {
    uint32_t cr0, cr3;
    __asm__ volatile ("mov %%cr0, %0" : "=r"(cr0));
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    if ((cr0 & 0x80010000u) != 0x80010000u || cr3 != (uint32_t)directory)
        return 0;
    /* Do not attempt a destructive test unless the page tables say safe. */
    uint32_t ro = (uint32_t)&_text_start;
    if ((tables[ro >> 22][(ro >> 12) & 1023] & 7u) != 1u ||
        (tables[0][0] & 1u) || (tables[0][0x1EF] & 1u))
        return 0;
    for (uint32_t d = 0; d < 32; d++) {
        if (directory[d] & 4u) return 0;
        for (uint32_t p = 0; p < 1024; p++)
            if (tables[d][p] & 4u) return 0;
    }
    int a = probe(0, 0);
    int b = probe(0x1EF000u, 0);
    int c = probe(ro, 1);
    return a && b && c;
}
uint32_t memory_faults_caught(void) { return caught; }
