#pragma once

#include <telkin/Telkin.h>

namespace tk {

    struct GenericHook {
        tk::DataMagic magic;
        u8 _[tk::cHookSize - sizeof(magic)];
    };
    
    // void KernelCopyData(uint32_t dst, uint32_t src, uint32_t len)
    using writefunc_t = void (*)(void* dst, void* src, u32 len);
    extern writefunc_t sPrivilegedWrite;

} // namespace tk
