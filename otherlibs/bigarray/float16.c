/*
  Fast Half Float Conversions
  Jeroen van der Zijp
  ftp://ftp.fox-toolkit.org/pub/fasthalffloatconversion.pdf
*/

#include "float16.h"

static unsigned int mantissatable[2048];
static unsigned int exponenttable[64];
static unsigned short offsettable[64];

static unsigned int
convertmantissa(unsigned int i)
{
    unsigned int m = i << 13;   /* Zero pad mantissa bits */
    unsigned int e = 0;         /* Zero exponent */
    while(!(m & 0x00800000)) {  /* While not normalized */
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

    mantissatable[0] = 0;
    for (i = 1; i < 1024; i++) mantissatable[i] = convertmantissa(i);
    for (; i < 2048; i++) mantissatable[i] = 0x38000000 + ((i - 1024) << 13);

    exponenttable[0] = 0;
    exponenttable[31] = 0x47800000;
    exponenttable[32] = 0x80000000;
    exponenttable[63] = 0xC7800000;
    for (i = 1; i < 31; i++) exponenttable[i] = i << 23;
    for (; i < 63; i++) exponenttable[i] = 0x80000000 + ((i - 32) << 23);

    offsettable[0] = 0;
    offsettable[32] = 0;
    for (i = 1; i < 32; i++) offsettable[i] = 1024;
    for (; i < 64; i++) offsettable[i] = 1024;
}

caml_ba_uint16
float16_of_float32(float)
{
    /* TODO */
    return 0;
}

float
float16_to_float32(caml_ba_uint16 h)
{
    return mantissatable[offsettable[h >> 10] + (h & 0x3ff)] +
        exponenttable[h >> 10];
}
