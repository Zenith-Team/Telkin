#pragma once

#include <dynamic_libs/os_types.h>
#include <type_traits>
#include <telkin/Preprocessor.h>

namespace tk {
    enum class DataMagic : u32 {
        BranchHook  = 0x01C0FFEE,
        PointerHook = 0x02C0FFEE,
        PatchHook   = 0x03C0FFEE,
        NullHook    = 0x04C0FFEE,
    };

    constexpr int cHookSize = 0x20;

    enum class BranchType : u32 {
        b,
        bl,
    };

    struct BranchHook {
        DataMagic magic;
        u32* source;
        const char* target;
        BranchType type;
        const u64* titleID;
        u32 reserved2;
        u32 reserved3;
        u32 reserved4;
    };

    static_assert(sizeof(BranchHook) == cHookSize, "BranchHook size mismatch");
    static_assert(std::is_trivially_constructible<BranchHook>::value, "BranchHook is not trivially constructible");
    static_assert(std::is_trivially_destructible<BranchHook>::value, "BranchHook is not trivially destructible");

    struct PointerHook {
        DataMagic magic;
        u32* source;
        const char* target;
        u32 isData; // bool
        const u64* titleID;
        u32 reserved2;
        u32 reserved3;
        u32 reserved4;
    };

    static_assert(sizeof(PointerHook) == cHookSize, "PointerHook size mismatch");
    static_assert(std::is_trivially_constructible<PointerHook>::value, "PointerHook is not trivially constructible");
    static_assert(std::is_trivially_destructible<PointerHook>::value, "PointerHook is not trivially destructible");

    struct PatchHook {
        DataMagic magic;
        void* addr;
        u16 count;
        u16 dataSize; // in bits
        const void* data;
        const u64* titleID;
        u32 reserved2;
        u32 reserved3;
        u32 reserved4;
    };

    static_assert(sizeof(PatchHook) == cHookSize, "PatchHook size mismatch");
    static_assert(std::is_trivially_constructible<PatchHook>::value, "PatchHook is not trivially constructible");
    static_assert(std::is_trivially_destructible<PatchHook>::value, "PatchHook is not trivially destructible");
    
    struct NullHook {
        DataMagic magic;
        u8 padding[cHookSize - sizeof(DataMagic)];
    };
    
    static_assert(sizeof(NullHook) == cHookSize, "NullHook size mismatch");
    static_assert(std::is_trivially_constructible<NullHook>::value, "NullHook is not trivially constructible");
    static_assert(std::is_trivially_destructible<NullHook>::value, "NullHook is not trivially destructible");
}

#ifdef __clangd__
    #define tMangle(...) PP_STR(__VA_ARGS__)
#else
    #define tMangle(...) __builtin_mangle(__VA_ARGS__) // red hills compiler magic
#endif

// Normal func
#define _tBranch3(addr, target, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), tMangle(target), type, nullptr, 0, 0, 0)

// Overloaded func
#define _tBranch4(addr, target, sig, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), tMangle(target, sig), type, nullptr, 0, 0, 0)

#define tBranch(...) \
    PP_CONCAT_VAL(_tBranch, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

// Normal func, titleid pinned
#define _tBranchPin4(addr, target, type, titleid) \
    const u64 _tPin_ ## addr = titleid; \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), tMangle(target), type, &_tPin_ ## addr, 0, 0, 0)

// Overloaded func, titleid pinned
#define _tBranchPin5(addr, target, sig, type, titleid) \
    const u64 _tPin_ ## addr = titleid; \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), tMangle(target, sig), type, &_tPin_ ## addr, 0, 0, 0)

#define tBranchPin(...) \
    PP_CONCAT_VAL(_tBranchPin, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

// Explicit mangled string
#define tBranchEx(addr, targetSym, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), targetSym, type, nullptr, 0, 0, 0)

// Explicit mangled string, titleid pinned
#define tBranchPinEx(addr, targetSym, type, titleid) \
    const u64 _tPin_ ## addr = titleid; \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), targetSym, type, &_tPin_ ## addr, 0, 0, 0)

// Normal func
#define _tPointerCode2(addr, target) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), tMangle(target), false, nullptr, 0, 0, 0)

// Overloaded func
#define _tPointerCode3(addr, target, sig) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), tMangle(target, sig), false, nullptr, 0, 0, 0)

#define tPointerCode(...) \
    PP_CONCAT_VAL(_tPointerCode, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

// Normal func, titleid pinned
#define _tPointerCodePin3(addr, target, titleid) \
    const u64 _tPin_ ## addr = titleid; \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), tMangle(target), false, &_tPin_ ## addr, 0, 0, 0)

// Overloaded func, titleid pinned
#define _tPointerCodePin4(addr, target, sig) \
    const u64 _tPin_ ## addr = titleid; \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), tMangle(target, sig), false, &_tPin_ ## addr, 0, 0, 0)

#define tPointerCode(...) \
    PP_CONCAT_VAL(_tPointerCode, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

#define tPointerCodePin(...) \
    PP_CONCAT_VAL(_tPointerCodePin, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

#define tPointerData(addr, target) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), tMangle(target), true, nullptr, 0, 0, 0)

// Explicit mangled string
#define tPointerEx(addr, targetSym, isdata) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), targetSym, isdata, nullptr, 0, 0, 0)

// Explicit mangled string, titleid pinned
#define tPointerPinEx(addr, targetSym, isdata, titleid) \
    const u64 _tPin_ ## addr = titleid; \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), targetSym, isdata, &_tPin_ ## addr, 0, 0, 0)

    #define _tPatch_u(addr, bits, ...) \
    const u##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), nullptr, 0, 0, 0)

#define _tPatch_s(addr, bits, ...) \
    const s##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), nullptr, 0, 0, 0)

#define _tPatch_f(addr, bits, ...) \
    const f##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), nullptr, 0, 0, 0)

#define _tPatchPin_u(addr, bits, titleid, ...) \
    const u##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    const u64 _tPin_ ## addr = titleid; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), &_tPin_ ## addr, 0, 0, 0)

#define _tPatchPin_s(addr, bits, titleid, ...) \
    const s##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    const u64 _tPin_ ## addr = titleid; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), &_tPin_ ## addr, 0, 0, 0)

#define _tPatchPin_f(addr, bits, titleid, ...) \
    const f##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    const u64 _tPin_ ## addr = titleid; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), &_tPin_ ## addr, 0, 0, 0)

#define tPatch8u(addr, ...) _tPatch_u(addr, 8, __VA_ARGS__)
#define tPatch16u(addr, ...) _tPatch_u(addr, 16, __VA_ARGS__)
#define tPatch32u(addr, ...) _tPatch_u(addr, 32, __VA_ARGS__)
#define tPatch64u(addr, ...) _tPatch_u(addr, 64, __VA_ARGS__)

#define tPatch8s(addr, ...) _tPatch_s(addr, 8, __VA_ARGS__)
#define tPatch16s(addr, ...) _tPatch_s(addr, 16, __VA_ARGS__)
#define tPatch32s(addr, ...) _tPatch_s(addr, 32, __VA_ARGS__)
#define tPatch64s(addr, ...) _tPatch_s(addr, 64, __VA_ARGS__)

#define tPatch32f(addr, ...) _tPatch_f(addr, 32, __VA_ARGS__)
#define tPatch64f(addr, ...) _tPatch_f(addr, 64, __VA_ARGS__)

#define tPatchNop(addr) tPatch32u(addr, 0x60000000)
#define tPatchBlr(addr) tPatch32u(addr, 0x4E800020)

#define tPatchPin8u(addr, titleid, ...) _tPatchPin_u(addr, 8, titleid, __VA_ARGS__)
#define tPatchPin16u(addr, titleid, ...) _tPatchPin_u(addr, 16, titleid, __VA_ARGS__)
#define tPatchPin32u(addr, titleid, ...) _tPatchPin_u(addr, 32, titleid, __VA_ARGS__)
#define tPatchPin64u(addr, titleid, ...) _tPatchPin_u(addr, 64, titleid, __VA_ARGS__)

#define tPatchPin8s(addr, titleid, ...) _tPatchPin_s(addr, 8, titleid, __VA_ARGS__)
#define tPatchPin16s(addr, titleid, ...) _tPatchPin_s(addr, 16, titleid, __VA_ARGS__)
#define tPatchPin32s(addr, titleid, ...) _tPatchPin_s(addr, 32, titleid, __VA_ARGS__)
#define tPatchPin64s(addr, titleid, ...) _tPatchPin_s(addr, 64, titleid, __VA_ARGS__)

#define tPatchPin32f(addr, titleid, ...) _tPatchPin_f(addr, 32, titleid, __VA_ARGS__)
#define tPatchPin64f(addr, titleid, ...) _tPatchPin_f(addr, 64, titleid, __VA_ARGS__)

#define tPatchPinNop(addr, titleid) tPatchPin32u(addr, titleid, 0x60000000)
#define tPatchPinBlr(addr, titleid) tPatchPin32u(addr, titleid, 0x4E800020)
