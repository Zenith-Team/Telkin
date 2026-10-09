#include <new>
#include <cafe.h>
#include <cstdarg>
#include <climits>
#include <cmath>
#include <limits>

#define sprintf(str, format, ...) \
    __os_snprintf(str, INT_MAX, format, ##__VA_ARGS__)

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wwritable-strings"

#include <semver.c>

#pragma clang diagnostic pop

// stuff Telkin needs
#define TK_IMPL_MEMCMP
#define TK_IMPL_ABORT
#define TK_IMPL_ISDIGIT
#define TK_IMPL_STRPBRK
#define TK_IMPL_STRCMP
#define TK_IMPL_ISSPACE
#define TK_IMPL_MEMMOVE
#define TK_IMPL_STRCAT
#define TK_IMPL_MEMSET
#define TK_IMPL_STRNCMP
#define TK_IMPL_FREE
#define TK_IMPL_STRTOL
#define TK_IMPL_OPERATOR_DELETE
#define TK_IMPL_MEMCPY
#define TK_IMPL_OPERATOR_NEW
#define TK_IMPL_STRCHR
#define TK_IMPL_STRCPY
#define TK_IMPL_CALLOC
#define TK_IMPL_STRLEN
#define TK_IMPL_ISALPHA
#define TK_IMPL_ISUPPER
#include <telkin/Runtime.h>

// libcpp
namespace std {
    inline namespace __1 {
        [[noreturn]] void __libcpp_verbose_abort(const char* fmt, ...) {
            OSReport("VerboseAbort: %s", fmt);
            abort();
        }
    }
}

// compiler-rt
extern "C" u64 __fixunssfdi(float a) {
    if (a <= 0.0f)
        return 0;
    double da = a;
    u32 high = da / 4294967296.f;               // da / 0x1p32f;
    u32 low = da - (double)high * 4294967296.f; // high * 0x1p32f;
    return ((u64)high << 32) | low;
}

extern "C" s64 __fixsfdi(float a) {
    if (a < 0.0f) {
        return -__fixunssfdi(-a);
    }
    return __fixunssfdi(a);
}

extern "C" u64 __fixunsdfdi(double a) {
    if (a <= 0.0)
        return 0;
    u32 high = a / 4294967296.f;               // a / 0x1p32f;
    u32 low = a - (double)high * 4294967296.f; // high * 0x1p32f;
    return ((u64)high << 32) | low;
}

extern "C" s64 __fixdfdi(double a) {
    if (a < 0.0) {
        return -__fixunsdfdi(-a);
    }
    return __fixunsdfdi(a);
}

extern "C" double __floatdidf(s64 a) {
    static const double twop52 = 4503599627370496.0; // 0x1.0p52
    static const double twop32 = 4294967296.0;       // 0x1.0p32

    union {
        int64_t x;
        double d;
    } low = {.d = twop52};

    const double high = (int32_t)(a >> 32) * twop32;
    low.x |= a & 0x00000000ffffffffLL;

    const double result = (high - twop52) + low.d;
    return result;
}

extern "C" float __floatdisf(s64 a) {
    if (a == 0)
        return 0.0;

    enum {
        dstMantDig = 23 + 1,
        srcBits = sizeof(s64) * 8,
        srcIsSigned = ((s64)-1) < 0,
    };

    const s64 s = srcIsSigned ? a >> (srcBits - 1) : 0;

    a = (u64)(a ^ s) - s;
    int sd = srcBits - __builtin_clzll(a); // number of significant digits
    int e = sd - 1;                        // exponent
    if (sd > dstMantDig) {
        //  start:  0000000000000000000001xxxxxxxxxxxxxxxxxxxxxxPQxxxxxxxxxxxxxxxxxx
        //  finish: 000000000000000000000000000000000000001xxxxxxxxxxxxxxxxxxxxxxPQR
        //                                                12345678901234567890123456
        //  1 = msb 1 bit
        //  P = bit dstMantDig-1 bits to the right of 1
        //  Q = bit dstMantDig bits to the right of 1
        //  R = "or" of all bits to the right of Q
        if (sd == dstMantDig + 1) {
        a <<= 1;
        } else if (sd == dstMantDig + 2) {
        // Do nothing.
        } else {
        a = ((u64)a >> (sd - (dstMantDig + 2))) |
            ((a & ((u64)(-1) >> ((srcBits + dstMantDig + 2) - sd))) != 0);
        }
        // finish:
        a |= (a & 4) != 0; // Or P into R
        ++a;               // round - this step may add a significant bit
        a >>= 2;           // dump Q and R
        // a is now rounded to dstMantDig or dstMantDig+1 bits
        if (a & ((u64)1 << dstMantDig)) {
        a >>= 1;
        ++e;
        }
        // a is now rounded to dstMantDig bits
    } else {
        a <<= (dstMantDig - sd);
        // a is now rounded to dstMantDig bits
    }
    const int dstBits = sizeof(float) * 8;
    const u32 dstSignMask = 1U << (dstBits - 1);
    const int dstExpBits = dstBits - 23 - 1;
    const int dstExpBias = (1 << (dstExpBits - 1)) - 1;
    const u32 dstSignificandMask = (1U << 23) - 1;
    // Combine sign, exponent, and mantissa.
    const u32 result = ((u32)s & dstSignMask) |
                        ((u32)(e + dstExpBias) << 23) |
                        ((u32)(a)&dstSignificandMask);

    const union {
        float f;
        u32 i;
    } rep = {.i = result};
    return rep.f;
}

extern "C" float __floatundisf(u64 a) {
    return __floatdisf(static_cast<s64>(a));
}

extern "C" double __floatundidf(u64 a) {
    static const double twop52 = 4503599627370496.0;           // 0x1.0p52
    static const double twop84 = 19342813113834066795298816.0; // 0x1.0p84
    static const double twop84_plus_twop52 =
        19342813118337666422669312.0; // 0x1.00000001p84

    union {
        uint64_t x;
        double d;
    } high = {.d = twop84};
    union {
        uint64_t x;
        double d;
    } low = {.d = twop52};

    high.x |= a >> 32;
    low.x |= a & 0x00000000ffffffffULL;

    const double result = (high.d - twop84_plus_twop52) + low.d;
    return result;
}

typedef int si_int;
typedef unsigned su_int;

typedef long long di_int;
typedef unsigned long long du_int;

typedef union {
  di_int all;
  struct {
    si_int high;
    su_int low;
  } s;
} dwords;

typedef union {
  du_int all;
  struct {
    su_int high;
    su_int low;
  } s;
} udwords;


du_int __udivmoddi4(du_int a, du_int b, du_int *rem) {
    const unsigned n_uword_bits = sizeof(su_int) * CHAR_BIT;
    const unsigned n_udword_bits = sizeof(du_int) * CHAR_BIT;
    udwords n;
    n.all = a;
    udwords d;
    d.all = b;
    udwords q;
    udwords r;
    unsigned sr;
    // special cases, X is unknown, K != 0
    if (n.s.high == 0) {
        if (d.s.high == 0) {
        // 0 X
        // ---
        // 0 X
        if (rem)
            *rem = n.s.low % d.s.low;
        return n.s.low / d.s.low;
        }
        // 0 X
        // ---
        // K X
        if (rem)
        *rem = n.s.low;
        return 0;
    }
    // n.s.high != 0
    if (d.s.low == 0) {
        if (d.s.high == 0) {
        // K X
        // ---
        // 0 0
        if (rem)
            *rem = n.s.high % d.s.low;
        return n.s.high / d.s.low;
        }
        // d.s.high != 0
        if (n.s.low == 0) {
        // K 0
        // ---
        // K 0
        if (rem) {
            r.s.high = n.s.high % d.s.high;
            r.s.low = 0;
            *rem = r.all;
        }
        return n.s.high / d.s.high;
        }
        // K K
        // ---
        // K 0
        if ((d.s.high & (d.s.high - 1)) == 0) /* if d is a power of 2 */ {
        if (rem) {
            r.s.low = n.s.low;
            r.s.high = n.s.high & (d.s.high - 1);
            *rem = r.all;
        }
        return n.s.high >> __builtin_ctz(d.s.high);
        }
        // K K
        // ---
        // K 0
        sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
        // 0 <= sr <= n_uword_bits - 2 or sr large
        if (sr > n_uword_bits - 2) {
        if (rem)
            *rem = n.all;
        return 0;
        }
        ++sr;
        // 1 <= sr <= n_uword_bits - 1
        // q.all = n.all << (n_udword_bits - sr);
        q.s.low = 0;
        q.s.high = n.s.low << (n_uword_bits - sr);
        // r.all = n.all >> sr;
        r.s.high = n.s.high >> sr;
        r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
    } else /* d.s.low != 0 */ {
        if (d.s.high == 0) {
        // K X
        // ---
        // 0 K
        if ((d.s.low & (d.s.low - 1)) == 0) /* if d is a power of 2 */ {
            if (rem)
            *rem = n.s.low & (d.s.low - 1);
            if (d.s.low == 1)
            return n.all;
            sr = __builtin_ctz(d.s.low);
            q.s.high = n.s.high >> sr;
            q.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
            return q.all;
        }
        // K X
        // ---
        // 0 K
        sr = 1 + n_uword_bits + __builtin_clz(d.s.low) - __builtin_clz(n.s.high);
        // 2 <= sr <= n_udword_bits - 1
        // q.all = n.all << (n_udword_bits - sr);
        // r.all = n.all >> sr;
        if (sr == n_uword_bits) {
            q.s.low = 0;
            q.s.high = n.s.low;
            r.s.high = 0;
            r.s.low = n.s.high;
        } else if (sr < n_uword_bits) /* 2 <= sr <= n_uword_bits - 1 */ {
            q.s.low = 0;
            q.s.high = n.s.low << (n_uword_bits - sr);
            r.s.high = n.s.high >> sr;
            r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
        } else /* n_uword_bits + 1 <= sr <= n_udword_bits - 1 */ {
            q.s.low = n.s.low << (n_udword_bits - sr);
            q.s.high = (n.s.high << (n_udword_bits - sr)) |
                    (n.s.low >> (sr - n_uword_bits));
            r.s.high = 0;
            r.s.low = n.s.high >> (sr - n_uword_bits);
        }
        } else {
        // K X
        // ---
        // K K
        sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
        // 0 <= sr <= n_uword_bits - 1 or sr large
        if (sr > n_uword_bits - 1) {
            if (rem)
            *rem = n.all;
            return 0;
        }
        ++sr;
        // 1 <= sr <= n_uword_bits
        // q.all = n.all << (n_udword_bits - sr);
        q.s.low = 0;
        if (sr == n_uword_bits) {
            q.s.high = n.s.low;
            r.s.high = 0;
            r.s.low = n.s.high;
        } else {
            q.s.high = n.s.low << (n_uword_bits - sr);
            r.s.high = n.s.high >> sr;
            r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
        }
        }
    }
    // Not a special case
    // q and r are initialized with:
    // q.all = n.all << (n_udword_bits - sr);
    // r.all = n.all >> sr;
    // 1 <= sr <= n_udword_bits - 1
    su_int carry = 0;
    for (; sr > 0; --sr) {
        // r:q = ((r:q)  << 1) | carry
        r.s.high = (r.s.high << 1) | (r.s.low >> (n_uword_bits - 1));
        r.s.low = (r.s.low << 1) | (q.s.high >> (n_uword_bits - 1));
        q.s.high = (q.s.high << 1) | (q.s.low >> (n_uword_bits - 1));
        q.s.low = (q.s.low << 1) | carry;
        // carry = 0;
        // if (r.all >= d.all)
        // {
        //      r.all -= d.all;
        //      carry = 1;
        // }
        const di_int s = (di_int)(d.all - r.all - 1) >> (n_udword_bits - 1);
        carry = s & 1;
        r.all -= d.all & s;
    }
    q.all = (q.all << 1) | carry;
    if (rem)
        *rem = r.all;
    return q.all;
}

u64 __udivdi3(u64 a, u64 b) {
    return __udivmoddi4(a, b, 0);
}

di_int __divdi3(di_int a, di_int b) {
    const int bits_in_dword_m1 = (int)(sizeof(di_int) * CHAR_BIT) - 1;
    di_int s_a = a >> bits_in_dword_m1;                   // s_a = a < 0 ? -1 : 0
    di_int s_b = b >> bits_in_dword_m1;                   // s_b = b < 0 ? -1 : 0
    a = (a ^ s_a) - s_a;                                  // negate if s_a == -1
    b = (b ^ s_b) - s_b;                                  // negate if s_b == -1
    s_a ^= s_b;                                           // sign of quotient
    return (__udivmoddi4(a, b, (du_int *)0) ^ s_a) - s_a; // negate if s_a == -1
}

__umoddi3(du_int a, du_int b) {
    du_int r;
    __udivmoddi4(a, b, &r);
    return r;
}

__moddi3(di_int a, di_int b) {
    const int bits_in_dword_m1 = (int)(sizeof(di_int) * CHAR_BIT) - 1;
    di_int s = b >> bits_in_dword_m1; // s = b < 0 ? -1 : 0
    b = (b ^ s) - s;                  // negate if s == -1
    s = a >> bits_in_dword_m1;        // s = a < 0 ? -1 : 0
    a = (a ^ s) - s;                  // negate if s == -1
    du_int r;
    __udivmoddi4(a, b, &r);
    return ((di_int)r ^ s) - s; // negate if s == -1
}