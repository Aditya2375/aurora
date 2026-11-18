/* 64-bit division helpers gcc emits calls to in freestanding 32-bit code.
 * Shift-and-subtract long division; correctness over speed. */

unsigned long long __udivmoddi4(unsigned long long num,
                                unsigned long long den,
                                unsigned long long *rem) {
    unsigned long long quot = 0;
    unsigned long long qbit = 1;

    if (den == 0)
        return 0;                   /* freestanding: divide-by-zero yields 0 */

    while ((long long)den >= 0 && den < num) {
        den <<= 1;
        qbit <<= 1;
    }
    while (qbit) {
        if (den <= num) {
            num -= den;
            quot += qbit;
        }
        den >>= 1;
        qbit >>= 1;
    }
    if (rem)
        *rem = num;
    return quot;
}

unsigned long long __udivdi3(unsigned long long num, unsigned long long den) {
    return __udivmoddi4(num, den, 0);
}

unsigned long long __umoddi3(unsigned long long num, unsigned long long den) {
    unsigned long long rem = 0;
    __udivmoddi4(num, den, &rem);
    return rem;
}

/* negate in the unsigned domain: -LLONG_MIN is undefined in signed C */
static unsigned long long abs64(long long v) {
    return v < 0 ? 0ull - (unsigned long long)v : (unsigned long long)v;
}

long long __divdi3(long long num, long long den) {
    int neg = (num < 0) != (den < 0);
    long long q = (long long)__udivmoddi4(abs64(num), abs64(den), 0);
    return neg ? (long long)(0ull - (unsigned long long)q) : q;
}

long long __moddi3(long long num, long long den) {
    unsigned long long rem = 0;
    int neg = num < 0;
    __udivmoddi4(abs64(num), abs64(den), &rem);
    return neg ? (long long)(0ull - rem) : (long long)rem;
}
