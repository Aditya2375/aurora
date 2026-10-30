#ifndef AURORA_KPRINT_H
#define AURORA_KPRINT_H

/* Format subset: %s %c %d %u %x %% with the usual C widths, plus %lld
 * %llu %llx for 64-bit values. The printf format attribute lets gcc
 * check every call site: a plain int passed to %d just works, and a
 * mismatched argument is a build error. */
__attribute__((format(printf, 1, 2)))
void kprintf(const char *fmt, ...);
__attribute__((format(printf, 2, 3)))
void kprintf_attr(unsigned char attr, const char *fmt, ...);

/* VGA attributes used by the shell UI */
#define ATTR_TEXT   0x07
#define ATTR_DIM    0x08
#define ATTR_ACCENT 0x03
#define ATTR_OK     0x0A
#define ATTR_WARN   0x0E
#define ATTR_ERR    0x0C
#define ATTR_TITLE  0x0B

#endif
