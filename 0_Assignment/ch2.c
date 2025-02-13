#include<limits.h>
#include<stdlib.h>

/* Ex. 2.58: 
 * This function is to test wether your mechine is little endian
 * return 1 if is, or return 0 if not 
 */
int is_little_endian()
{
    int x = 1, i;
    char *p = (char*)&x;
    for(i = 0; i != sizeof(i) && *p++ != 1; ++i);
    return i == 0;
}

/* Ex. 2.59
 * Generate a new word using the lowest byte from x, and
 * the remained bytes from y
 */
unsigned generate_word(unsigned x, unsigned y)
{
    return (x & 0xFF) + (y & ~0xFF);
}

/* Ex. 2.60
 * replace the ith btye in unsigned number x by b
 * return the new unsigned number
 */
unsigned replace_byte(unsigned x, int i, unsigned char b)
{
    if(i >= 0 && i < sizeof(unsigned)){//overflow check
        unsigned mask = ~(0xFF << (i << 3)), y = b << (i << 3);
        x = (x & mask) +  y;
    }
    return x;
}

/* Ex. 2.61*/
int number_check(int x)
{
    return !x
        || !~x
        || !((x & 0xFF) ^ 0xFF)
        || !(((x >> ((sizeof(int)-1)<<3)) & 0xFF) ^ 0);
}

/* Ex. 2.62
 * Test whether the right shift on your machine is arithmetic
 * return 1 if is, or return 0 if not
 */
int int_shifts_are_arithmetic()
{
    return !~(INT_MIN >> ((sizeof(int) << 3) - 1));
}

/* Ex. 2.63
 * Perform shift arthemetically 
 */
unsigned srl(unsigned x, int k)
{
    unsigned xsra = (int) x >> k;
    int mask = (-1) << ((sizeof(int) << 3) - k);
    return xsra &~ mask;
}

/* Ex. 2.63
 * Perform shift logically
 */
int sra(int x, int k)
{
    int xsrl = (unsigned) x >> k;
    int mask1 = 1 << ((sizeof(int) << 3) - k - 1);
    int mask2 = (-1) << ((sizeof(int) << 3) - k - 1);//overflow if xsrl is minus.
    return ((mask1 ^ xsrl) + mask2) | xsrl;
}

/* Ex. 2.64
 * Return 1 when any odd bit of x equals 1; 0 otherwise. 
 * Assume w = 32 
 */
int any_odd_one(unsigned x)
{
    unsigned mask = 0x55555555;//0101 0101 0101 0101 0101 0101 0101 0101
    return x & mask;
}

/* Ex. 2.65
 * return 1 when x contains an odd number of 1s; 0 otherwise. 
 * Assume w = 32
 */
int odd_ones(unsigned x)
{
    x ^= x >> 16;
    x ^= x >> 8;
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    x &= 1;         /* xor all bits at rightmost bit */

    return x;
}

/* Ex. 2.66
 * Generate mask indicating leftmost 1 in x.  Assume w=32.
 * For example, 0xFF00 -> 0x8000, and 0x6600 --> 0x4000.
 * If x = 0, then return 0.
 */
int leftmost_one(unsigned x)
{
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x -= x >> 1;

    return x;
}

/* Ex. 2.68
 * Mask with least significant n bits set to 1
 * Example: n = 6 --> 0x3F, n = 17 --> 0x1FFFF
 * Assume 1 <= n <= w
 */
int lower_one_mask(int n)
{
    n = 1 << (n - 1);
    return n | (n - 1);
}

/* Ex. 2.69
 * Do rotating left shift. Assume 0 <= n < w
 * Example when x = 0x12345678 and w = 32
 *     n = 4 --> 0x23456781, n = 20 --> 0x67812345
 */
unsigned rotate_left(unsigned x, int n)
{
    return (x << n) |
           (unsigned)((unsigned long long)x >> ((sizeof(unsigned) << 3) - n));
}

/* Ex. 2.70
 * Return 1 when x can be represented as an n-bit, 2's-complement
 * number; 0 otherwise
 * Assume 1 <= n <= w
 */
int fits_bits(int x, int n)
{
    int mask = ~((1 << (n - 1)) - 1);
    return ((x & mask) ^ mask) == 0 || (x & mask) == 0;
}

/* Ex. 2.71
 * Extract byte from word. Return as signed integer
 */
typedef unsigned packed_t;
int xbyte(packed_t word, int bytenum)
{
    return (int)(word << ((3 - bytenum) << 3)) >> 24;
}

/* Ex. 2.73
 * Addition that asturates to TMin or TMax
 */
int saturating_add(int x, int y)
{
    int rsh_bit = (sizeof(int) << 3) - 1;
    int x_sign = (x & INT_MIN) >> rsh_bit;
    int xy_sign = x_sign ^ ((y & INT_MIN) >> rsh_bit);
    int sum = x + y;
    int xsum_sign = x_sign ^ ((sum & INT_MIN) >> rsh_bit); 

    // temp1 == 0 -> no overflow
    // temp1: == -1 -> overflow
    int temp1 = (~xy_sign & (xy_sign ^ xsum_sign));

    return (sum & ~temp1) + (temp1 & (x_sign ^ INT_MAX));
}

/* Ex. 2.74
 * Determine whether arguments can be subtracted without overflow
 */
int tsub_ok(int x, int y)
{
    int x_sign = x & INT_MIN;
    int xy_sign = x_sign ^ (y & INT_MIN);
    int xsub_sign = x_sign ^ ((x - y) & INT_MIN);

    return (xy_sign & xsub_sign) == 0;
}

/* Ex. 2.75
 * Calculate the high w bit of the production of unsigned int x and unsigned int y
 */
int signed_high_prod(int x, int y);
unsigned unsigned_high_prod(unsigned x, unsigned y)
{
    return signed_high_prod(x, y);
}

/* Ex. 2.76
 * Implementation for standard calloc
 */
void *my_calloc(size_t nmemb, size_t size)
{
    return NULL;
}

/* Ex. 2.77
 * quick multiplication by given factor K
 */
void quick_multiply(int x)
{
    int A = x + (x << 4); // K = 17
    int B = x - (x << 3); // K = -7
    int C = (x << 6) - (x << 2); // K = 60
    int D = (x << 4) - (x << 7); // K = -112
}

/* Ex. 2.78
 * Divide by power 2. Assume 0 <= k < w -1
 */
int divide_power2(int x, int k)
{
    int mask = x >> ((sizeof(int) << 3) - 1);
    return (x + (mask & ((1 << k) - 1))) >> k;
}

/* Ex. 2.79
 * Calculate 3 * x/4. May overflow when 3 * x
 */
int mul3div4(int x)
{
    x = x + x + x;
    int mask = x >> ((sizeof(int) << 3) - 1);
    return (x + (mask & 3)) >> 2;
}

/* Ex. 2.80
 * Calculate 3 * x/4. Never overflow
 */
int threefourths(int x)
{
    int mask = ~(x >> ((sizeof(int) << 3) - 1));
    int fourth = ((x & ~3) >> 2) + (((x & 3) + (mask & 3)) >> 2);
    return x + ~fourth + 1;
}

/* Ex. 2.81
 * generate bits 
 */
void generate_bits(unsigned j, unsigned k)
{
    unsigned A = ~((1 << k) - 1);
    unsigned B = ((1 << (j + k)) - 1) ^ ((1 << j) - 1);
}

/* floating-point part */
/* Access bit-level representation floating-point number*/
typedef unsigned float_bits;

static const unsigned FLOAT_W = 32;
static const unsigned FLOAT_FRAC_W = 23;
static const unsigned FLOAT_SIGN_MASK = 1 << (FLOAT_W - 1);
static const unsigned FLOAT_FRAC_MASK = (1 << FLOAT_FRAC_W) - 1;
static const unsigned FLOAT_EXP_MASK = ~0 ^ FLOAT_SIGN_MASK ^ FLOAT_FRAC_MASK;
static const unsigned FLOAT_EXP_BIAS = (1 << (FLOAT_W - FLOAT_FRAC_W - 2)) - 1;
#define GET_SIGN(x) (x >> (FLOAT_W - 1))
#define GET_EXP_BITS(x) ((x & FLOAT_EXP_MASK) >> FLOAT_FRAC_W)
#define GET_FRAC_BITS(x) (x & FLOAT_FRAC_MASK)
#define IS_NAN(x) (GET_EXP_BITS(x) == ((1 << (FLOAT_W - FLOAT_FRAC_W - 1)) - 1) && GET_FRAC_BITS(x) != 0)
#define MAKE_FLOAT_BITS(sign, exp, frac) ((sign << (FLOAT_W - 1)) | (exp << FLOAT_FRAC_W) | frac)
#define ROUND_2_EVEN(x, k) (((x + ((x & (1 << (k - 1))) & ((x & (1 << k)) >> 1))) >> (k - 1)) << (k - 1))

/* Ex. 2.92
 * Compute -f. If f is NaN, then return f.
 */
float_bits float_negate(float_bits f)
{
    if (IS_NAN(f))
    {
        return f;
    }

    return f ^ FLOAT_SIGN_MASK;
}

/* Ex. 2.93
 * Compute |f|. If f is NaN, then return f.
 */
float_bits float_absval(float_bits f)
{
    if (IS_NAN(f))
    {
        return f;
    }

    return f & ~FLOAT_SIGN_MASK;
}

/* Ex. 2.94
 * Compute 2.0 * f. If f is NaN, then return f.
 */
float_bits float_twice(float_bits f)
{
    float_bits sign = GET_SIGN(f);
    float_bits exp = GET_EXP_BITS(f);
    float_bits frac = GET_FRAC_BITS(f);

    if (exp == 0xFF)
    {
        return f;
    }

    if (exp > 0)
    {
        exp++;

        if (exp >= 0xFF)
        {
            exp = 0xFF;
            frac = 0;
        }
    }
    else
    {
        frac <<= 1;
    }

    return MAKE_FLOAT_BITS(sign, exp, frac);
}

/* Ex. 2.95
 * Compute 0.5 * f. If f is NaN, then return f.
 */
float_bits float_half(float_bits f)
{
    float_bits sign = GET_SIGN(f);
    float_bits exp = GET_EXP_BITS(f);
    float_bits frac = GET_FRAC_BITS(f);

    if (exp == 0xFF)
    {
        return f;
    }

    if (exp > 0)
    {
        exp--;

        if (exp == 0)
        {
            frac = ROUND_2_EVEN(frac, 1);
            frac >>= 1;
            frac += 0x400000;
        }
    }
    else
    {
        frac = ROUND_2_EVEN(frac, 1);
        frac >>= 1;
    }

    return MAKE_FLOAT_BITS(sign, exp, frac);
}

/* Ex. 2.96
 * Compute (int)f. If conversion causes overflow or f is Nan, return 0x80000000.
 * ROUND to Zero
 */
int float_f2i(float_bits f)
{
    float_bits sign, exp, frac;
    float_bits round_bit, round_mask;
    float_bits ret;

    sign = f & FLOAT_SIGN_MASK;
    exp = (f & FLOAT_EXP_MASK) >> FLOAT_FRAC_W;
    frac = f & FLOAT_FRAC_MASK;

    if (exp >= FLOAT_W - 1 + FLOAT_EXP_BIAS)
    {
        return FLOAT_SIGN_MASK;
    }

    if (exp < FLOAT_EXP_BIAS)
    {
        return 0;
    }

    exp -= FLOAT_EXP_BIAS;
    frac += (1 << FLOAT_FRAC_W);

    int result1 = frac << (exp - FLOAT_FRAC_W);
    int result2 = frac >> (FLOAT_FRAC_W - exp);

    ret = exp >= FLOAT_FRAC_W ? result1 : result2;

    return sign ? -ret : ret;
}

/* Ex. 2.97
 * Compute (float) i. Assume 32-bit int
 */
float_bits float_i2f(int i)
{
    int sign = GET_SIGN(i);

    if ( (i & ~(1 << (FLOAT_W - 1))) == 0)
    {
        return MAKE_FLOAT_BITS(sign, (sign & (FLOAT_EXP_BIAS + FLOAT_W - 1)), 0);
    }

    i = (i & ~sign) | (-i & sign);

    int count = 0;

    {
        int x = i;
        int temp, mask;

        x |= x >> 1;
        x |= x >> 2;
        x |= x >> 4;
        x |= x >> 8;
        x |= x >> 16;
        x -= x >> 1;

        mask = 0xFF;
        mask |= mask << 8;
        mask = ~mask;
        temp = x & mask;
        count += (!!temp) << 4;

        mask = 0xFF;
        mask |= mask << 16;
        mask = ~mask;
        temp = x & mask;
        count += (!!temp) << 3;

        mask = 0xF;
        mask |= mask << 8;
        mask |= mask << 16;
        mask = ~mask;
        temp = x & mask;
        count += (!!temp) << 2;

        mask = 0x33;
        mask |= mask << 8;
        mask |= mask << 16;
        mask = ~mask;
        temp = x & mask;
        count += (!!temp) << 1;

        mask = 0x55;
        mask |= mask << 8;
        mask |= mask << 16;
        mask = ~mask;
        temp = x & mask;
        count += !!temp;

        mask = ~0;
        temp = x & mask;
        count += !!temp;
    }

    int exp = count + FLOAT_EXP_BIAS - 1;

    if (count <= FLOAT_FRAC_W + 1)
    {
        i <<= FLOAT_FRAC_W + 1 - count;
    }
    else
    {
        // Round to even
        int round_bit = 1 << (count - FLOAT_FRAC_W - 2);
        int round_mask = round_bit - 1;

        if ( (i & round_bit) == 0 || ((i & round_mask) == 0 && ((i >> 1) & round_bit) == 0 ))
        {
            i >>= count - FLOAT_FRAC_W - 1;
        }
        else
        {
            i >>= count - FLOAT_FRAC_W - 1;
            i += 1;
            exp += i >> (FLOAT_FRAC_W + 1);
        }
    }

    i &= (1 << FLOAT_FRAC_W) - 1;

    return MAKE_FLOAT_BITS(sign, exp, i);
}
