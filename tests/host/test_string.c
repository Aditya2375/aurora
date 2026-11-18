#include "harness.h"
#include <string.h>             /* host reference for comparison */
#include "../../kernel/lib/string.h"

int main(void) {
    char a[64], b[64];

    memset(a, 'A', sizeof(a));
    memcpy(b, a, sizeof(b));
    CHECK(memcmp(a, b, sizeof(a)) == 0, "memcpy/memcmp round trip");

    memset(a, 0, sizeof(a));
    for (int i = 0; i < 64; i++) a[i] = (char)i;
    memmove(a + 4, a, 40);                       /* overlapping, forward */
    for (int i = 0; i < 40; i++) CHECK(a[4 + i] == (char)i, "memmove forward");
    memmove(a, a + 4, 40);                       /* overlapping, backward */
    for (int i = 0; i < 40; i++) CHECK(a[i] == (char)i, "memmove backward");

    CHECK(strlen("") == 0, "strlen empty");
    CHECK(strlen("aurora") == 6, "strlen basic");
    CHECK(strnlen("abc", 10) == 3, "strnlen under max");
    CHECK(strnlen("abcdef", 3) == 3, "strnlen capped");
    CHECK(strcmp("abc", "abc") == 0, "strcmp equal");
    CHECK(strcmp("abc", "abd") < 0, "strcmp less");
    CHECK(strcmp("abd", "abc") > 0, "strcmp greater");
    CHECK(strcmp("", "a") < 0, "strcmp empty");
    CHECK(strncmp("abcdef", "abcxyz", 3) == 0, "strncmp prefix equal");
    CHECK(strncmp("abcdef", "abcxyz", 4) != 0, "strncmp diverge");
    CHECK(strncmp("abc", "abcdef", 6) < 0, "strncmp length");

    strcpy(a, "kernel");
    CHECK(strcmp(a, "kernel") == 0, "strcpy");
    memset(a, 'X', sizeof(a));
    strncpy(a, "os", 8);
    CHECK(strcmp(a, "os") == 0 && a[7] == '\0', "strncpy zero fills");
    memset(a, 'X', sizeof(a));
    strncpy(a, "longer than eight", 8);
    CHECK(memcmp(a, "longer t", 8) == 0, "strncpy truncates without NUL");

    memset(a, 0xAB, sizeof(a));
    CHECK(((unsigned char *)memset(a, 0, 4)) == (unsigned char *)a, "memset returns dst");
    CHECK(a[0] == 0 && a[3] == 0 && (unsigned char)a[4] == 0xAB, "memset bounds");

    TEST_RESULTS();
}
