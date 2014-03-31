/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SHARED_BITUTIL_H_
#define SOURCE_INCLUDE_SHARED_BITUTIL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Code is in public domain from aggregate.org */
static inline unsigned int
bit_ones32(register unsigned int x)
{
    /*
     * 32-bit recursive reduction using SWAR... but first step is mapping
     * 2-bit values into sum of 2 1-bit values in sneaky way
     */
    x -= ((x >> 1) & 0x55555555);
    x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
    x = (((x >> 4) + x) & 0x0f0f0f0f);
    x += (x >> 8);
    x += (x >> 16);
    return(x & 0x0000003f);
}

static inline int
bit_hibit(unsigned int x)
{
    unsigned int num, i, hi;
    static const unsigned int shft[] = { 16, 8, 4, 2, 1, 0 },
                              mask[] = { 0xffff, 0xff, 0xf, 0x3, 0x1 };

    num = 0;
    for (i = 0; shft[i]; i++) {
        hi = x >> shft[i];
        if (hi) {
            x = hi;
            num += shft[i];
        } else {
            x = x & mask[i];
        }
    }
    return (num + x);
}

static inline unsigned int
bit_msb32(register unsigned int x)
{
    x |= (x >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    return (x & ~(x >> 1));
}

#ifdef __cplusplus
}
#endif
#endif /* SOURCE_INCLUDE_SHARED_BITUTIL_H_ // NOLINT */
