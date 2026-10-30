#include "kprint.h"
#include "vga.h"
#include "serial.h"
#include <stdarg.h>

static void u64_to_str(unsigned long long v, char *buf, int base, int *len) {
    char tmp[32];
    int i = 0;
    if (v == 0) {
        buf[(*len)++] = '0';
        return;
    }
    while (v > 0) {
        unsigned d = (unsigned)(v % (unsigned)base);
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        v /= (unsigned)base;
    }
    while (i > 0)
        buf[(*len)++] = tmp[--i];
}

static void kvprintf(unsigned char attr, const char *fmt, va_list ap) {
    unsigned char saved = vga_get_attr();
    vga_set_attr(attr);
    char out[512];
    int len = 0;

    for (const char *p = fmt; *p && len < (int)sizeof(out) - 16; p++) {
        if (*p != '%') {
            out[len++] = *p;
            continue;
        }
        p++;
        if (!*p)
            break;
        int llong = 0;
        if (*p == 'l' && p[1] == 'l') {
            llong = 1;
            p += 2;
            if (!*p)
                break;
        }
        switch (*p) {
        case 's': {
            const char *s = va_arg(ap, const char *);
            if (!s)
                s = "(null)";
            while (*s && len < (int)sizeof(out) - 16)
                out[len++] = *s++;
            break;
        }
        case 'c':
            out[len++] = (char)va_arg(ap, int);
            break;
        case 'd': {
            long long v = llong ? va_arg(ap, long long) : va_arg(ap, int);
            /* negate in the unsigned domain: INT64_MIN has no positive twin */
            unsigned long long uv = v < 0 ? 0ull - (unsigned long long)v
                                          : (unsigned long long)v;
            if (v < 0)
                out[len++] = '-';
            u64_to_str(uv, out, 10, &len);
            break;
        }
        case 'u':
            u64_to_str(llong ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned),
                       out, 10, &len);
            break;
        case 'x':
            u64_to_str(llong ? va_arg(ap, unsigned long long) : va_arg(ap, unsigned),
                       out, 16, &len);
            break;
        case '%':
            out[len++] = '%';
            break;
        default:
            out[len++] = '%';
            out[len++] = *p;
            break;
        }
    }
    out[len] = '\0';
    vga_write(out);
    serial_write(out);
    vga_set_attr(saved);
}

void kprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kvprintf(ATTR_TEXT, fmt, ap);
    va_end(ap);
}

void kprintf_attr(unsigned char attr, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    kvprintf(attr, fmt, ap);
    va_end(ap);
}
