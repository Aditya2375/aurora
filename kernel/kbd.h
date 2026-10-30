#ifndef AURORA_KBD_H
#define AURORA_KBD_H

#include <stdbool.h>
#include <stdint.h>

void kbd_init(void);
bool kbd_poll(char *c);     /* pop one character; false when buffer empty */
char kbd_getc(void);        /* block until a character is typed */

#endif
