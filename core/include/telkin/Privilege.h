#pragma once

#include <dynamic_libs/os_types.h>

namespace tk {
    // void KernelCopyData(uint32_t dst, uint32_t src, uint32_t len)
    using writefunc_t = void (*)(const void* dst, const void* src, u32 len);
    extern writefunc_t privilegedWrite;
}
