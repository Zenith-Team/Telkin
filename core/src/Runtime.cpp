#include <new>
#include <cafe.h>

//------
// These are internal and blocked from exports by Tachyon
void* operator new(std::size_t size) {
    return MEMAllocFromDefaultHeap(size);
}

void operator delete(void* ptr) noexcept {
    return MEMFreeToDefaultHeap(ptr);
}

//------

extern "C" void* memcpy(void* dest, const void* src, size_t n) {
    return OSBlockMove(dest, src, n, 0);
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
