#ifndef AURORA_KPRINT_H
#define AURORA_KPRINT_H

/* Format subset: %s %c %d %u %x %%. Numeric specifiers always read a
 * (signed/unsigned) long long from the varargs - cast arguments, as every
 * kernel call site does. This keeps 64-bit values printable with no
 * length modifiers. */
void kprintf(const char *fmt, ...);
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
