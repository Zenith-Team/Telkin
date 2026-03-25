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

//------
// These are internal and blocked from exports by Tachyon
void* operator new(std::size_t size) {
    return MEMAllocFromDefaultHeap(size);
}

void operator delete(void* ptr) noexcept {
    return MEMFreeToDefaultHeap(ptr);
}

extern "C" void free(void* ptr) {
    MEMFreeToDefaultHeap(ptr);
}

extern "C" void* memalign(size_t align, size_t size) {
    return MEMAllocFromDefaultHeapEx(size, align);
}

extern "C" void* memchr(const void* p, int ch, size_t count) {
    for (size_t i = 0; i < count; i++) {
        const u8 b = reinterpret_cast<const u8*>(p)[i];
        if (b == (u8)ch) {
            return (void*) (&reinterpret_cast<const u8*>(p)[i]);
        }
    }

    return nullptr;
}

extern "C" void* calloc(size_t num, size_t size) {
    void* p = MEMAllocFromDefaultHeap(num * size);
    memset(p, 0, num * size);
    return p;
}

//------

extern "C" char* strcat(char* dest, const char* src) {
    char* ptr = dest;

    while (*ptr != '\0') {
        ptr++;
    }

    while (*src != '\0') {
        *ptr = *src;
        ptr++;
        src++;
    }

    *ptr = '\0';
    
    return dest;
}

extern "C" bool isspace(char c) { return (c == ' '); }

extern "C" bool isdigit(char c) { return (c >= '0' && c <= '9'); }

extern "C" bool isupper(char c) { return (c >= 'A' && c <= 'Z'); }

extern "C" bool isalpha(char c) { return ((isupper(c) || (c >= 'a' && c <= 'z'))); }

extern "C" long strtol(const char *nptr, char **endptr, int base) {
    const char* s = nptr;
    unsigned long acc;
    int c;
    unsigned long cutoff;
    int cutlim;
    int any;
    int neg = 0;

    while (isspace((unsigned char)*s)) {
        s++;
    }

    if (*s == '-') {
        neg = 1;
        s++;
    } else if (*s == '+') {
        s++;
    }

    if ((base == 0 || base == 16) &&
        *s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X')) {
        s += 2;
        base = 16;
    }
    if (base == 0) {
        base = *s == '0' ? 8 : 10;
    }

    acc = neg ? -(unsigned long)std::numeric_limits<long>::min() : std::numeric_limits<long>::max();
    cutoff = acc / (unsigned long)base;
    cutlim = acc % (unsigned long)base;

    acc = 0;
    any = 0;

    for (;; s++) {
        c = *s;

        if (isdigit(c))
            c -= '0';
        else if (isalpha(c))
            c -= (isupper(c) ? 'A' : 'a') - 10;
        else
            break;

        if (c >= base)
            break;

        if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim)) {
            any = -1;
        } else {
            any = 1;
            acc *= base;
            acc += c;
        }
    }

    if (any < 0) {
        acc = neg ? std::numeric_limits<long>::min() : std::numeric_limits<long>::max();
    } else if (neg) {
        acc = -acc;
    }

    if (endptr != 0) {
        *endptr = (char *)(any ? s : nptr);
    }

    return acc;
}

extern "C" char* strpbrk(const char* str, const char* charset) {
    if (!str || !charset) {
        return NULL;
    }
    
    // For each character in the string
    while (*str) {
        // Check if it matches any character in the charset
        const char* c = charset;
        while (*c) {
            if (*str == *c) {
                return (char*)str;  // Return pointer to first matching character
            }
            c++;
        }
        str++;
    }
    
    return NULL;  // No match found
}

extern "C" void* memcpy(void* dest, const void* src, size_t n) {
    return OSBlockMove(dest, src, n, true);
}

extern "C" int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

extern "C" size_t strlen(const char* s) {
    const char* p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

extern "C" void* memset(void* dst, int value, size_t size) {
    return OSBlockSet(dst, value, size);
}

extern "C" void* memmove(void* dst, const void* src, size_t size) {
    return OSBlockMove(dst, src, size, true);
}

extern "C" int memcmp(const void* a, const void* b, size_t size) {
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;

    while (size--) {
        if (*pa != *pb)
            return *pa - *pb;
        pa++;
        pb++;
    }
    return 0;
}

namespace std {
    inline namespace __1 {
        [[noreturn]] void __libcpp_verbose_abort(const char* fmt, ...) {
            OSReport("VerboseAbort: %s", fmt);
            abort();
        }
    }
}

extern "C" void abort() {
    OSFatal("abort() called");
}

extern "C" char* strchr(const char* s1, int i) {
    const unsigned char* s = (const unsigned char*)s1;
    unsigned char c = i;

    while (*s && *s != c) s++;
    if (*s == c) return (char*)s;
    return nullptr;
}

extern "C" char* strcpy(char* dst0, const char* src0) {
    char* s = dst0;
    while ((*dst0++ = *src0++));
    return s;
}

extern "C" int strncmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) return 0;

    while (n-- != 0 && *s1 == *s2) {
        if (n == 0 || *s1 == '\0') break;
        s1++;
        s2++;
    }
    return (*(unsigned char*)s1) - (*(unsigned char*)s2);
}

extern "C" char* strncpy(char* __restrict dst0, const char* __restrict src0, size_t count) {
    char* dscan;
    const char* sscan;

    dscan = dst0;
    sscan = src0;
    while (count > 0) {
        --count;
        if ((*dscan++ = *sscan++) == '\0') break;
    }
    while (count-- > 0) *dscan++ = '\0';
    return dst0;
}

extern "C" char* strstr(const char* hs, const char* ne) {
    size_t i;
    int c = ne[0];

    if (c == 0) return (char*)hs;

    for (; hs[0] != '\0'; hs++) {
        if (hs[0] != c) continue;
        for (i = 1; ne[i] != 0; i++)
            if (hs[i] != ne[i]) break;
        if (ne[i] == '\0') return (char*)hs;
    }
    return NULL;
}

/* Math cstdlib funcs*/

extern "C" float fabsf(float a) {
    return (a > 0) ? a : -a;
}

extern "C" double fabs(double a) {
    return (a > 0) ? a : -a;
}

extern "C" double atof(const char *s) {
    double a = 0.0;
    int e = 0;
    int c;
    int sign = 1;

    // Skip whitespace
    while (*s == ' ' || *s == '\t' || *s == '\n') {
        s++;
    }

    // Handle sign
    if (*s == '+') {
        s++;
    } else if (*s == '-') {
        sign = -1;
        s++;
    }

    while ((c = *s++) != '\0' && isdigit(c)) {
        a = a * 10.0 + (c - '0');
    }

    if (c == '.') {
        while ((c = *s++) != '\0' && isdigit(c)) {
            a = a * 10.0 + (c - '0');
            e = e - 1;
        }
    }

    if (c == 'e' || c == 'E') {
        int exp_sign = 1;
        int i = 0;
        c = *s++;

        if (c == '+') {
            c = *s++;
        } else if (c == '-') {
            c = *s++;
            exp_sign = -1;
        }

        while (isdigit(c)) {
            i = i * 10 + (c - '0');
            c = *s++;
        }

        e += i * exp_sign;
    }

    while (e > 0) {
        a *= 10.0;
        e--;
    }

    while (e < 0) {
        a *= 0.1;
        e++;
    }

    return a * sign;
}

/* String stdlib functions */

// toupper
#define _U 01
#define _L 02
#define _N 04
#define _S 010
#define _P 020
#define _C 040
#define _X 0100
#define _B 0200

#define _CTYPE_DATA_0_127                                                     \
    _C, _C, _C, _C, _C, _C, _C, _C, _C, _C | _S, _C | _S, _C | _S, _C | _S,   \
        _C | _S, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C, _C,  \
        _C, _C, _C, _S | _B, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P, _P,  \
        _P, _P, _P, _N, _N, _N, _N, _N, _N, _N, _N, _N, _N, _P, _P, _P, _P,   \
        _P, _P, _P, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U | _X, _U, \
        _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U, _U,   \
        _U, _U, _P, _P, _P, _P, _P, _P, _L | _X, _L | _X, _L | _X, _L | _X,   \
        _L | _X, _L | _X, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, _L, \
        _L, _L, _L, _L, _L, _L, _L, _P, _P, _P, _P, _C

#define _CTYPE_DATA_128_255                                                    \
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   \
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

const char _ctype_[1 + 256] = {0, _CTYPE_DATA_0_127,
                                        _CTYPE_DATA_128_255};

#define __locale_ctype_ptr() _ctype_
#define __CTYPE_PTR (__locale_ctype_ptr())

extern "C" int islower(int c) {
    return ((__CTYPE_PTR[c + 1] & (_U | _L)) == _L);
}

extern "C" int toupper(int c) {
    return islower(c) ? c - 'a' + 'A' : c;
}

// from compiler-rt

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

#define TELKIN_REGISTERS
#include <telkin/Assembly.h>

extern "C" int printf(const char* format, ...) tAssembly(
    // r2 is persistent/unused
    lis r2, OSReport@ha;
    lwz r2, OSReport@l(r2);
    mtctr r2;
    mflr r2;
    bctrl;
    mtlr r2;
    li r3, 0;
    blr;
)
