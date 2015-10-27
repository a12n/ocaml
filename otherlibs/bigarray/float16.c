/*
  Fast Half Float Conversions
  Jeroen van der Zijp
  ftp://ftp.fox-toolkit.org/pub/fasthalffloatconversion.pdf
*/

#include "float16.h"

static uint32_t mantissatable[2048];
static uint32_t exponenttable[64];
static caml_ba_uint16 offsettable[64];

static caml_ba_uint16 basetable[512];
static unsigned char shifttable[512];

static uint32_t
convertmantissa(uint32_t i)
{
    uint32_t m = i << 13;       /* Zero pad mantissa bits */
    uint32_t e = 0;             /* Zero exponent */
    while (!(m & 0x00800000)) { /* While not normalized */
        e -= 0x00800000;        /* Decrement exponent (1<<23) */
        m <<= 1;                /* Shift mantissa */
    }
    m &= ~0x00800000;           /* Clear leading 1 bit */
    e += 0x38800000;            /* Adjust bias ((127-14)<<23) */
    return m | e;               /* Return combined number */
}

void
float16_init(void)
{
    int i;

    /* Generate tables for half to single conversion */

    mantissatable[0] = 0;
    for (i = 1; i < 1024; i++) mantissatable[i] = convertmantissa(i);
    for (i = 1024; i < 2048; i++)
        mantissatable[i] = 0x38000000 + ((i - 1024) << 13);

    exponenttable[0] = 0;
    exponenttable[31] = 0x47800000;
    exponenttable[32] = 0x80000000;
    exponenttable[63] = 0xC7800000;
    for (i = 1; i < 31; i++) exponenttable[i] = i << 23;
    for (i = 33; i < 63; i++) exponenttable[i] = 0x80000000 + ((i - 32) << 23);

    offsettable[0] = 0;
    offsettable[32] = 0;
    for (i = 1; i < 32; i++) offsettable[i] = 1024;
    for (i = 33; i < 64; i++) offsettable[i] = 1024;

    /* Generate tables for single to half conversion */

    for (i = 0; i < 256; i++) {
        int e;

        e = i - 127;
        if (e < - 24) {         /* Very small numbers map to zero */
            basetable[i | 0x000] = 0x0000;
            basetable[i | 0x100] = 0x8000;
            shifttable[i | 0x000] = 24;
            shifttable[i | 0x100] = 24;
        } else if (e < -14) {   /* Small numbers map to denorms */
            basetable[i | 0x000] = (0x0400 >> (-e - 14));
            basetable[i | 0x100] = (0x0400 >> (-e - 14)) | 0x8000;
            shifttable[i | 0x000] = -e - 1;
            shifttable[i | 0x100] = -e - 1;
        } else if (e <= 15) { /* Normal numbers just lose precision */
            basetable[i | 0x000] = ((e + 15) << 10);
            basetable[i | 0x100] = ((e + 15) << 10) | 0x8000;
            shifttable[i | 0x000] = 13;
            shifttable[i | 0x100] = 13;
        } else if (e < 128) {   /* Large numbers map to Infinity */
            basetable[i | 0x000] = 0x7C00;
            basetable[i | 0x100] = 0xFC00;
            shifttable[i | 0x000] = 24;
            shifttable[i | 0x100] = 24;
        } else {      /* Infinity and NaN's stay Infinity and NaN's */
            basetable[i | 0x000] = 0x7C00;
            basetable[i | 0x100] = 0xFC00;
            shifttable[i | 0x000] = 13;
            shifttable[i | 0x100] = 13;
        }
    }
}

caml_ba_uint16
float16_of_float32(float f)
{
    union { float f; uint32_t i; } v;
    v.f = f;
    return basetable[(v.i >> 23) & 0x1ff] +
        ((v.i & 0x007fffff) >> shifttable[(v.i >> 23) & 0x1ff]);
}

float
float16_to_float32(caml_ba_uint16 h)
{
    return mantissatable[offsettable[h >> 10] + (h & 0x3ff)] +
        exponenttable[h >> 10];
}
