#ifndef _RAND_H_

#define _RAND_H_
#define SIM_RAND_MAX         32767

/*
 * This random generators are implementing
 * by following POSIX.1-2001 directives.
 */

extern __thread unsigned long next;

static long simRandom(void) {
    register long x, hi, lo, t;

    /*
     * Compute x[n + 1] = (7^5 * x[n]) mod (2^31 - 1).
     * From "Random number generators: good ones are hard to find",
     * Park and Miller, Communications of the ACM, vol. 31, no. 10,
     * October 1988, p. 1195.
     */
    x = (long)next;
    hi = x / 127773;
    lo = x % 127773;
    t = 16807 * lo - 2836 * hi;
    if (t <= 0) {
        t += 0x7fffffff;
    }
    next = (unsigned long)t;
    return (unsigned long)t;
}

inline static void simSRandom(unsigned long seed) {
    next = seed;
}

/*
 * In Numerical Recipes in C: The Art of Scientific Computing
 * (William H. Press, Brian P. Flannery, Saul A. Teukolsky, William T. Vetterling;
 *  New York: Cambridge University Press, 1992 (2nd ed., p. 277))
 */
inline static long simRandomRange(long low, long high) {
    return low + (long) ( ((double) high)* (simRandom() / (SIM_RAND_MAX + 1.0)));
}

#endif
