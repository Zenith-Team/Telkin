#pragma once

#include "types.h"

#define _tLoaderSectionNameData ".loaderdata"
#define _tLoaderSectionNameRodata ".loaderconst"

#define _tLoaderSectionStart                                                                                        \
    CUSTOM_SECTION(data, _tLoaderSectionNameData);                                                                            \
    CUSTOM_SECTION(rodata, _tLoaderSectionNameRodata)

#define _tLoaderSectionEnd                                                                                          \
    CUSTOM_SECTION(data, default);                                                                                  \
    CUSTOM_SECTION(rodata, default)

namespace tloader {
    ENUM_CLASS(DataMagic,
        BranchHook = 0x01C0FFEE,
        PointerHook = 0x02C0FFEE,
        Patch = 0x03C0FFEE,
    );
}

namespace tloader {
    class BranchHook {
    public:
        enum Type {
            Type_b,
            Type_bl,
        };

    public:
        u32 magic;
        u32* source;
        const char* target;
        Type type;
    };

    class PointerHook {
    public:
        u32 magic;
        u32* source;
        const char* target;
        u32 isData;
    };
}

static_assert(sizeof(tloader::BranchHook) == sizeof(tloader::PointerHook), "BranchHook <-> PointerHook size mismatch");

#define HOOK_PREFIX _tHook_

#define tHook(addr, target, type) \
    _tLoaderSectionStart;               \
    tloader::BranchHook TOKENPASTE2(HOOK_PREFIX, addr) = { tloader::DataMagic::BranchHook, (u32*)addr, target, type }; \
    _tLoaderSectionEnd

#define tPointer(addr, target) \
    _tLoaderSectionStart;            \
    tloader::PointerHook TOKENPASTE2(HOOK_PREFIX, addr) = { tloader::DataMagic::PointerHook, (u32*)addr, target, 0 }; \
    _tLoaderSectionEnd

namespace tloader {
    class Patch {
    public:
        u32 magic;
        void* addr;
        u16 count;
        u16 dataSize; // in bits
        void* data;
    };
}

#define PATCH_PREFIX _tPatch_

#define _tPatch(addr, bits, ...) \
    _tLoaderSectionStart;        \
    const u##bits TOKENPASTE2(PATCH_PREFIX, TOKENPASTE2(_, addr))[] = { __VA_ARGS__ }; \
    tloader::Patch TOKENPASTE2(PATCH_PREFIX, addr) = {                \
        tloader::DataMagic::Patch, (u32*)addr,                        \
        sizeof(TOKENPASTE2(PATCH_PREFIX, TOKENPASTE2(_, addr))) / sizeof(u##bits), \
        bits, (void*)&TOKENPASTE2(PATCH_PREFIX, TOKENPASTE2(_, addr)) \
    };                                                                \
    _tLoaderSectionEnd

#define tPatch16(addr, ...) _tPatch(addr, 16, __VA_ARGS__)
#define tPatch32(addr, ...) _tPatch(addr, 32, __VA_ARGS__)
#define tPatch64(addr, ...) _tPatch(addr, 64, __VA_ARGS__)
