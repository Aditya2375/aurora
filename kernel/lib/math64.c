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

long long __divdi3(long long num, long long den) {
    int neg = 0;
    if (num < 0) { num = -num; neg = !neg; }
    if (den < 0) { den = -den; neg = !neg; }
    long long q = (long long)__udivmoddi4((unsigned long long)num,
                                          (unsigned long long)den, 0);
    return neg ? -q : q;
}

long long __moddi3(long long num, long long den) {
    unsigned long long rem = 0;
    int neg = num < 0;
    if (neg) num = -num;
    if (den < 0) den = -den;
    __udivmoddi4((unsigned long long)num, (unsigned long long)den, &rem);
    return neg ? -(long long)rem : (long long)rem;
}
