#pragma once

#include "types.h"

extern "C" {

struct OsSpecifics {
    u32 addr_OSDynLoad_Acquire;
    u32 addr_OSDynLoad_FindExport;
    u32 addr_OSTitle_main_entry;

    u32 addr_KernSyscallTbl1;
    u32 addr_KernSyscallTbl2;
    u32 addr_KernSyscallTbl3;
    u32 addr_KernSyscallTbl4;
    u32 addr_KernSyscallTbl5;
};

extern s32 (*OSDynLoad_Acquire)(const char* rpl, u32* handle);
extern s32 (*OSDynLoad_FindExport)(u32 handle, s32 isdata, const char* symbol, void* address);
extern void* (*OSBlockSet)(void* dst, u8 value, size_t n);
extern void (*OSConsoleWrite)(const char* str, s32 len);
extern s32 (*__os_snprintf)(char* str, s32 size, const char* format, ...);

extern u32* MEMAllocFromDefaultHeapExPtr;
extern u32* MEMAllocFromDefaultHeapPtr;
extern u32* MEMFreeToDefaultHeapPtr;

#define MEMAllocFromDefaultHeapEx ((void* (*)(size_t size, int align))(*MEMAllocFromDefaultHeapExPtr))
#define MEMAllocFromDefaultHeap ((void* (*)(size_t))(*MEMAllocFromDefaultHeapPtr))
#define MEMFreeToDefaultHeap ((void (*)(void*))(*MEMFreeToDefaultHeapPtr))

}
