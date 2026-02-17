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

    constexpr int cHookSize = 0x10;

    enum class BranchType : u32 {
        b,
        bl,
    };

    struct BranchHook {
        DataMagic magic;
        u32* source;
        const char* target;
        BranchType type;
    };

    static_assert(sizeof(BranchHook) == cHookSize, "BranchHook size mismatch");
    static_assert(std::is_trivially_constructible<BranchHook>::value, "BranchHook is not trivially constructible");
    static_assert(std::is_trivially_destructible<BranchHook>::value, "BranchHook is not trivially destructible");

    struct PointerHook {
        DataMagic magic;
        u32* source;
        const char* target;
        u32 isData; // bool
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
    };

    static_assert(sizeof(PatchHook) == cHookSize, "PatchHook size mismatch");
    static_assert(std::is_trivially_constructible<PatchHook>::value, "PatchHook is not trivially constructible");
    static_assert(std::is_trivially_destructible<PatchHook>::value, "PatchHook is not trivially destructible");
}

// Normal func
#define _tBranch3(addr, target, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), __builtin_mangle(target), type);

// Overloaded func
#define _tBranch4(addr, target, sig, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), __builtin_mangle(target, sig), type);

#define tBranch(...) \
    PP_CONCAT_VAL(_tBranch, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

// Explicit mangled string
#define tBranchEx(addr, targetSym, type) \
    tk::BranchHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::BranchHook(tk::DataMagic::BranchHook, reinterpret_cast<u32*>(addr), targetSym, type);

// Normal func/var
#define _tPointer3(addr, target, isdata) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), __builtin_mangle(target), isdata);

// Overloaded func
#define _tPointer4(addr, target, sig, isdata) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), __builtin_mangle(target, sig), isdata);

#define tPointer(...) \
    PP_CONCAT_VAL(_tPointer, PP_NARG(__VA_ARGS__))(__VA_ARGS__)

// Explicit mangled string
#define tPointerEx(addr, targetSym, isdata) \
    tk::PointerHook _tHook_ ## addr __attribute__((section(".loaderdata"))) = tk::PointerHook(tk::DataMagic::PointerHook, reinterpret_cast<u32*>(addr), targetSym, isdata);

#define _tPatch(addr, bits, ...) \
    const u##bits _tPatch_Data_ ## addr [] = { __VA_ARGS__ }; \
    tk::PatchHook _tPatch_ ## addr __attribute__((section(".loaderdata"))) = tk::PatchHook(tk::DataMagic::PatchHook, reinterpret_cast<u32*>(addr), sizeof(_tPatch_Data_##addr) / sizeof(u##bits), bits, reinterpret_cast<const void*>(&_tPatch_Data_##addr));

#define tPatch8(addr, ...) _tPatch(addr, 8, __VA_ARGS__)
#define tPatch16(addr, ...) _tPatch(addr, 16, __VA_ARGS__)
#define tPatch32(addr, ...) _tPatch(addr, 32, __VA_ARGS__)
#define tPatch64(addr, ...) _tPatch(addr, 64, __VA_ARGS__)

#define tPatchNop(addr) tPatch32(addr, 0x60000000)
#define tPatchBlr(addr) tPatch32(addr, 0x4E800020)
