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
