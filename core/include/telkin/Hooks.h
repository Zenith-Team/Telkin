#pragma once

#include <dynamic_libs/os_types.h>
#include <type_traits>
#include <telkin/Preprocessor.h>

namespace tk {
    enum class DataMagic : u32 {
        BranchHook  = 0x01C0FFEE,
        PointerHook = 0x02C0FFEE,
        PatchHook   = 0x03C0FFEE,
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
        u32 reserved1;
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
        u32 reserved1;
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
        u32 reserved1;
        u32 reserved2;
        u32 reserved3;
        u32 reserved4;
    };

    static_assert(sizeof(PatchHook) == cHookSize, "PatchHook size mismatch");
    static_assert(std::is_trivially_constructible<PatchHook>::value, "PatchHook is not trivially constructible");
    static_assert(std::is_trivially_destructible<PatchHook>::value, "PatchHook is not trivially destructible");
}

#ifdef __clangd__
#define tMangle(...) PP_STR(__VA_ARGS__)
#else
#define tMangle(...) __builtin_mangle(__VA_ARGS__) // red hills compiler magic
#endif

// Normal func
#define _tBranch3(addr, target, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), tMangle(target), type, 0, 0, 0, 0);

// Overloaded func
#define _tBranch4(addr, target, sig, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), tMangle(target, sig), type, 0, 0, 0, 0);

#define tBranch(...) \
    PP_CONCAT_VAL(_tBranch, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

// Explicit mangled string
#define tBranchEx(addr, targetSym, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), targetSym, type, 0, 0, 0, 0);

// Normal func/var
#define _tPointer3(addr, target, isdata) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), tMangle(target), isdata, 0, 0, 0, 0);

// Overloaded func
#define _tPointer4(addr, target, sig, isdata) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), tMangle(target, sig), isdata, 0, 0, 0, 0);

#define tPointer(...) \
    PP_CONCAT_VAL(_tPointer, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

// Explicit mangled string
#define tPointerEx(addr, targetSym, isdata) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), targetSym, isdata, 0, 0, 0, 0);

#define _tPatch_u(addr, bits, ...) \
    const u##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), 0, 0, 0, 0);

#define _tPatch_s(addr, bits, ...) \
    const s##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), 0, 0, 0, 0);

#define _tPatch_f(addr, bits, ...) \
    const f##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr), 0, 0, 0, 0);

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
