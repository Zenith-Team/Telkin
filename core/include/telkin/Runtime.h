#pragma once

#include <cafe.h>
#include <cmath>
#include <cctype>
#include <cstdarg>
#include <climits>

#ifdef TK_IMPL_OPERATOR_NEW
void* operator new(std::size_t size) {
    return MEMAllocFromDefaultHeap(size);
}
#endif

#ifdef TK_IMPL_OPERATOR_DELETE
void operator delete(void* ptr) noexcept {
    return MEMFreeToDefaultHeap(ptr);
}
#endif

#ifdef TK_IMPL_FREE
extern "C" void free(void* ptr) {
    MEMFreeToDefaultHeap(ptr);
}
#endif

#ifdef TK_IMPL_CALLOC
extern "C" void* calloc(size_t num, size_t size) {
    void* p = MEMAllocFromDefaultHeap(num * size);
    memset(p, 0, num * size);
    return p;
}
#endif

#ifdef TK_IMPL_MEMALIGN
extern "C" void* memalign(size_t align, size_t size) {
    return MEMAllocFromDefaultHeapEx(size, align);
}
#endif

#ifdef TK_IMPL_MEMCHR
extern "C" void* memchr(const void* p, int ch, size_t count) {
    for (size_t i = 0; i < count; i++) {
        const u8 b = reinterpret_cast<const u8*>(p)[i];
        if (b == (u8)ch) {
            return (void*) (&reinterpret_cast<const u8*>(p)[i]);
        }
    }

    return nullptr;
}
#endif

#ifdef TK_IMPL_STRNCPY
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
#endif

#ifdef TK_IMPL_STRSTR
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
#endif

#ifdef TK_IMPL_MEMCPY
extern "C" void* memcpy(void* dest, const void* src, size_t n) {
    return OSBlockMove(dest, src, n, true);
}
#endif

#ifdef TK_IMPL_STRLEN
extern "C" size_t strlen(const char* s) {
    const char* p = s;
    while (*p) p++;
    return (size_t)(p - s);
}
#endif

#ifdef TK_IMPL_MEMSET
extern "C" void* memset(void* dst, int value, size_t size) {
    return OSBlockSet(dst, value, size);
}
#endif

#ifdef TK_IMPL_SPRINTF
extern "C" int sprintf(char* buffer, const char* format, ...) {
    __va_list va;
    va_start(va, format);
    const int ret = vsnprintf(buffer, INT_MAX, format, va);
    va_end(va);
    return ret;
}
#endif

#ifdef TK_IMPL_STRCHR
extern "C" char* strchr(const char* s1, int i) {
    const unsigned char* s = (const unsigned char*)s1;
    unsigned char c = i;

    while (*s && *s != c) s++;
    if (*s == c) return (char*)s;
    return nullptr;
}
#endif

#ifdef TK_IMPL_STRCPY
extern "C" char* strcpy(char* dst0, const char* src0) {
    char* s = dst0;
    while ((*dst0++ = *src0++));
    return s;
}
#endif

#ifdef TK_IMPL_FLOOR
extern "C" double floor(double x) {
    return floorf(x);
}
#endif

#ifdef TK_IMPL_CEIL
extern "C" double ceil(double x) {
    return ceilf(x);
}
#endif

#ifdef TK_IMPL_SIN
extern "C" double sin(double x) {
    return sinf(x);
}
#endif

#ifdef TK_IMPL_COS
extern "C" double cos(double x) {
    return cosf(x);
}
#endif

#ifdef TK_IMPL_ATAN2
extern "C" double atan2(double y, double x) {
    return atan2f(y, x);
}
#endif

#ifdef TK_IMPL_POW
extern "C" double pow(double x, double y) {
    return powf(x, y);
}
#endif

#ifdef TK_IMPL_LOG
extern "C" double log(double x) {
    return logf(x);
}
#endif

#ifdef TK_IMPL_ISSPACE
extern "C" int isspace(int c) {
    return (c == ' ');
}
#endif

#ifdef TK_IMPL_ISDIGIT
extern "C" int isdigit(int c) {
    return (c >= '0' && c <= '9');
}
#endif

#ifdef TK_IMPL_ISUPPER
extern "C" int isupper(int c) {
    return (c >= 'A' && c <= 'Z');
}
#endif

#ifdef TK_IMPL_ISALPHA
extern "C" int isalpha(int c) {
    return ((isupper(c) || (c >= 'a' && c <= 'z')));
}
#endif

#ifdef TK_IMPL_STRTOL
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
#endif

#ifdef TK_IMPL_STRNCMP
extern "C" int strncmp(const char* s1, const char* s2, size_t n) {
    if (n == 0) return 0;

    while (n-- != 0 && *s1 == *s2) {
        if (n == 0 || *s1 == '\0') break;
        s1++;
        s2++;
    }
    return (*(unsigned char*)s1) - (*(unsigned char*)s2);
}
#endif

#ifdef TK_IMPL_STRCAT
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
#endif

#ifdef TK_IMPL_STRPBRK
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
#endif

#ifdef TK_IMPL_STRCMP
extern "C" int strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) {
        a++;
        b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}
#endif

#ifdef TK_IMPL_MEMMOVE
extern "C" void* memmove(void* dst, const void* src, size_t size) {
    return OSBlockMove(dst, src, size, true);
}
#endif

#ifdef TK_IMPL_MEMCMP
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
#endif

#ifdef TK_IMPL_ABORT
extern "C" void abort() {
    OSFatal("abort() called");
}
#endif

#ifdef TK_IMPL_PRINTF
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
#endif

#ifdef TK_IMPL_ISLOWER
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

#ifndef __locale_ctype_ptr
#define __locale_ctype_ptr() _ctype_
#endif

#ifndef __CTYPE_PTR
#define __CTYPE_PTR (__locale_ctype_ptr())
#endif

extern "C" int islower(int c) {
    return ((__CTYPE_PTR[c + 1] & (_U | _L)) == _L);
}
#endif

#ifdef TK_IMPL_TOUPPER
extern "C" int toupper(int c) {
    return islower(c) ? c - 'a' + 'A' : c;
}
#endif

#ifdef TK_IMPL_FABSF
extern "C" float fabsf(float a) {
    return (a > 0) ? a : -a;
}
#endif

#ifdef TK_IMPL_FABS
extern "C" double fabs(double a) {
    return (a > 0) ? a : -a;
}
#endif

#ifdef TK_IMPL_ATOF
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
#endif
