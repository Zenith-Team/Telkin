#include <new>
#include <cafe.h>
#include <cstdarg>
#include <limits>

#define sprintf(str, format, ...) \
    __os_snprintf(str, (size_t)-1, format, ##__VA_ARGS__)

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

extern "C" void free(void* ptr) {
    MEMFreeToDefaultHeap(ptr);
}

extern "C" void* calloc(size_t num, size_t size) {
    void* p = MEMAllocFromDefaultHeap(num * size);
    memset(p, 0, num * size);
    return p;
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
