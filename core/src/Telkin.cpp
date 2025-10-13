#include <telkin/Telkin.h>

#include <dynamic_libs/os_functions.h>
#include <dynamic_libs/acp_functions.h>
#include <dynamic_libs/aoc_functions.h>
#include <dynamic_libs/ax_functions.h>
#include <dynamic_libs/fs_functions.h>
#include <dynamic_libs/gx2_functions.h>
#include <dynamic_libs/nfp_functions.h>
#include <dynamic_libs/nn_act_functions.h>
#include <dynamic_libs/nn_fp_functions.h>
#include <dynamic_libs/nn_nim_functions.h>
#include <dynamic_libs/nn_save_functions.h>
#include <dynamic_libs/ntag_functions.h>
#include <dynamic_libs/padscore_functions.h>
#include <dynamic_libs/proc_ui_functions.h>
#include <dynamic_libs/socket_functions.h>
#include <dynamic_libs/sys_functions.h>
#include <dynamic_libs/syshid_functions.h>
#include <dynamic_libs/vpad_functions.h>
#include <dynamic_libs/zlib_functions.h>
#include <utility>

/*
   ______    ____    _
  /_  __/__ / / /__ (_)__
   / / / -_) /  '_// / _ \
  /_/  \__/_/_/\_\/_/_//_/

    _              __             _                       _           _
   | |__ _  _     / /  __ ____ _ (_) __  __ ____ __   ___| |_    __ _| |
   | '_ \ || |   / /__/ // /  ' \/ / _ \/ // /\ \ /  / -_)  _|  / _` | |_
   |_.__/\_, |  /____/\_,_/_/_/_/_/_//_/\_, //_\_\   \___|\__|  \__,_|_(_)
          |__/                         /___/
*/

char logMsg[512];
s32 strlength(const char* str) { s32 len = 0; while (*str++) len++; return len; }
#define LOG(FMT, ...) do { __os_snprintf(logMsg, sizeof(logMsg), "" FMT "\n", ## __VA_ARGS__); OSConsoleWrite(logMsg, strlength(logMsg)); OSBlockSet(logMsg, 0, sizeof(logMsg)); } while (0)

using start_t = void (*)(u32, u32);

//using MemCopy_t = void (*)(void* dst, void* src, s32 bytes);
//MemCopy_t MemCopy = nullptr; // Kernel-privileged copy on console, standard memcpy on Cemu.

class HookList;
class FunctionList;

struct GenericHook {
    tk::DataMagic magic;
    u8 _[tk::cHookSize - sizeof(magic)];
};

namespace tk { // forward declaration
    bool loadRPL(const char* rplName, HookList& hookList, FunctionList& startFuncs);
    
    bool applyBranchHook(const tk::BranchHook* hook);
    bool applyPointerHook(const tk::PointerHook* hook);
    bool applyPatchHook(const tk::PatchHook* hook);
}

extern "C" {
    using funcPtr = void (*)();
    extern funcPtr __init_array_start[], __init_array_end[];
    
    void __rpl_crt() {} // Called by Cafe OS on acquire, don't do anything here
}

template <typename T>
class MyVector {
private:
    static constexpr u32 cGrowthFactor = 2;

public:
    MyVector()
        : mBuffer(nullptr)
        , mCount(0)
        , mCapacity(0)
    { }
    
    ~MyVector() {
        if (mBuffer != nullptr)
            MEMFreeToDefaultHeap(mBuffer);
    }
    
    MyVector(const MyVector&) = delete;
    MyVector& operator=(const MyVector&) = delete;

    void add(T entry) {
        mCount++;
        
        if (mBuffer == nullptr) {
            mCapacity = mCount * cGrowthFactor;
            mBuffer = (T*)MEMAllocFromDefaultHeap(mCapacity * sizeof(T));
            
            if (mBuffer == nullptr) {
                LOG("Sorry, out of memory. (A)");
            }
        } else if (mCount > mCapacity) {
                mCapacity = mCapacity * cGrowthFactor;
                
                T* newBuffer = (T*)MEMAllocFromDefaultHeap(mCapacity * sizeof(T));
                if (newBuffer == nullptr) {
                    LOG("Sorry, out of memory. (B)");
                }
                
                OSBlockMove(newBuffer, mBuffer, mCount * sizeof(T), false);
                
                MEMFreeToDefaultHeap(mBuffer);
                mBuffer = newBuffer;
        }
        
        mBuffer[mCount - 1] = entry;
    }
    
    [[nodiscard]] T* data() { return mBuffer; }
    [[nodiscard]] s32 count() const { return mCount; }
    
private:
    T* mBuffer;    // Storage
    s32 mCount;    // Full slots
    s32 mCapacity; // Total slots we have
};

class HookList {
public:
    void add(const void* hook, u32 startAddr, u32 endAddr) {
        const GenericHook* h = reinterpret_cast<const GenericHook*>(hook);
        
        const HookEntry entry = {
            .hook = h,
            .startAddr = startAddr,
            .endAddr = endAddr
        };
        
        mVector.add(entry);
    }
    
    bool validateRanges() {
        if (mVector.count() <= 1)
            return true;
        
        sort();
        
        for (s32 i = 1; i < mVector.count(); i++) {
            if (mVector.data()[i].startAddr < mVector.data()[i - 1].endAddr) {
                // TODO: Better diagnostic here with mod blame and addrs/types
                LOG("MOD INCOMPATIBILITY: Overlapping hooks found!");
                return false;
            }
        }
        
        LOG("No hook conflicts found :)");
        return true;
    }
    
    bool applyAll() {
        for (s32 i = 0; i < mVector.count(); i++) {
            const HookEntry& entry = mVector.data()[i];
            const GenericHook* hook = entry.hook;
            
            switch (hook->magic) {
                case tk::DataMagic::BranchHook: {
                    if (!tk::applyBranchHook(reinterpret_cast<const tk::BranchHook*>(hook)))
                        return false;
                    
                    break;
                }
                
                case tk::DataMagic::PatchHook: {
                    if (!tk::applyPatchHook(reinterpret_cast<const tk::PatchHook*>(hook)))
                        return false;
                    
                    break;
                }
                
                case tk::DataMagic::PointerHook: {
                    if (!tk::applyPointerHook(reinterpret_cast<const tk::PointerHook*>(hook)))
                        return false;
                    
                    break;
                }
            }
        }
        
        return true;
    }
    
private:
    void sort() {
        // TODO: This is bubble sort, optimize it later <3
        
        s32 n = mVector.count();
        do {
            s32 newN = 0;
            for (int i = 1; i <= (n-1); i++) {
                if (mVector.data()[i-1].startAddr > mVector.data()[i].startAddr) {
                    std::swap(mVector.data()[i-1], mVector.data()[i]);
                    newN = i;
                }
            }
            
            n = newN;
        } while (n > 1);
    }

private:
    struct HookEntry {
        const GenericHook* hook;
        u32 startAddr;
        u32 endAddr;
    };
    
    MyVector<HookEntry> mVector;
};

class FunctionList {
public:
    void add(start_t func) {
        mFuncs.add(func);
    }
    
    void callAll(u32 acquireAddr, u32 exportAddr) {
        for (s32 i = 0; i < mFuncs.count(); i++)
            mFuncs.data()[i](acquireAddr, exportAddr);
    }
    
private:
    MyVector<start_t> mFuncs;
};

extern "C" void init(u32 acquireAddr, u32 exportAddr, funcPtr callCtors) {
    static bool initialized = false;
    if (initialized)
      return;
    initialized = true;
    
    OS_SPECIFICS->addr_OSDynLoad_Acquire = acquireAddr;
    OS_SPECIFICS->addr_OSDynLoad_FindExport = exportAddr;
    
    for (funcPtr* p = __init_array_start; p != __init_array_end; p++) {
        (*p)();
    }

    InitOSFunctionPointers();
    InitACPFunctionPointers();
    InitAocFunctionPointers();
    // InitAXFunctionPointers(); // TODO: Fix this crash
    InitFSFunctionPointers();
    InitGX2FunctionPointers();
    InitNFPFunctionPointers();
    InitACTFunctionPointers();
    InitFpFunctionPointers();
    InitNimFunctionPointers();
    InitSaveFunctionPointers();
    InitNTAGFunctionPointers();
    InitPadScoreFunctionPointers();
    InitProcUIFunctionPointers();
    InitSocketFunctionPointers();
    InitSysFunctionPointers();
    InitSysHIDFunctionPointers();
    InitVPadFunctionPointers();
    InitZlibFunctionPointers();
    
    LOG("Telkin v0.1 by Zenith");

    FSInit();
    LOG("FS Inited");

    FSClient* client = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
    if (client == nullptr) {
        LOG("Error: Unable to allocate FSClient");
        return;
    }
    LOG("FSClient allocated");

    FSCmdBlock* cmd = (FSCmdBlock*)MEMAllocFromDefaultHeap(sizeof(FSCmdBlock));
    if (cmd == nullptr) {
        LOG("Error: Unable to allocate FSCmdBlock");
        MEMFreeToDefaultHeap(client);
        return;
    }
    LOG("FSCmdBlock allocated");

    constexpr int cBufferSize = 0x10000; // I hope rpl.txt won't exceed 10kb ;P
    u8* buffer = (u8*)MEMAllocFromDefaultHeapEx(cBufferSize, FS_IO_BUFFER_ALIGN);
    if (buffer == nullptr) {
        LOG("Error: Unable to allocate buffer");
        MEMFreeToDefaultHeap(client);
        MEMFreeToDefaultHeap(cmd);
        return;
    }
    LOG("Allocated buffer");

    OSBlockSet(buffer, 0, cBufferSize);
    LOG("OSBlockSet OK");

    FSAddClient(client, FS_RET_NO_ERROR);
    LOG("FSAddClient OK");
    FSInitCmdBlock(cmd);
    LOG("FSInitCmd OK");

    char path[FS_MAX_ARGPATH_SIZE];
    //strncpy(path, "/vol/content/rpl.txt", FS_MAX_ARGPATH_SIZE);
    __os_snprintf(path, sizeof(path), "/vol/content/rpl.txt");
    LOG("strncpy phobia overcame");

    FSFileHandle handle;
    FSOpenFile(client, cmd, path, "r", &handle, FS_RET_NO_ERROR);
    LOG("FSOpenFile OK");
    FSReadFile(client, cmd, buffer, 1, cBufferSize, handle, 0, FS_RET_NO_ERROR);
    LOG("FSReadFile OK");

    if (*buffer == 0) {
        LOG("Error: rpl.txt is empty or non-existent.");

        FSCloseFile(client, cmd, handle, FS_RET_NO_ERROR);
        MEMFreeToDefaultHeap(client);
        MEMFreeToDefaultHeap(cmd);
        MEMFreeToDefaultHeap(buffer);

        return;
    } else {
        LOG("rpl.txt was read");
    }
    
    { // Scoped to deallocate the memory used by HookList and FunctionList
        HookList hookList;
        FunctionList startFuncs;

        char* line = (char*)buffer;
        for (u32 i = 0; i < cBufferSize; i++) {
            if (buffer[i] == '\n') { // TODO: Support CRLF
                buffer[i] = '\0';
                
                tk::loadRPL(line, hookList, startFuncs);
                LOG("Finished loading RPL: %s", line);

                line = (char*)(buffer + i + 1);
            }
        }
        
        hookList.validateRanges();
        hookList.applyAll();
        
        startFuncs.callAll(OS_SPECIFICS->addr_OSDynLoad_Acquire, OS_SPECIFICS->addr_OSDynLoad_FindExport);
    }
    
    FSCloseFile(client, cmd, handle, FS_RET_NO_ERROR);
    LOG("FSCloseFile OK");
    MEMFreeToDefaultHeap(client);
    MEMFreeToDefaultHeap(cmd);
    MEMFreeToDefaultHeap(buffer);
    LOG("MEMFrees OK");

    LOG("Telkin is finished loading mods. Enjoy the game!");
    
    return;
}

namespace tk {

bool applyBranchHook(const tk::BranchHook* hook) {
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 instr = (reinterpret_cast<u32>(hook->target) - addr) & 0x03FFFFFC; // TODO: Validate range

    switch (hook->type) {
        default: {
            LOG("Invalid hook type for hook at: 0x%08X", addr);
            return false;
        }

        case tk::BranchType::b: {
            instr |= 0x48000000;
            break;
        }

        case tk::BranchType::bl: {
            instr |= 0x48000001;
            break;
        }
    }

    LOG("Writing branch hook: 0x%08X to 0x%08X", instr, addr);
    *hook->source = instr;

    DCFlushRange(hook->source, sizeof(instr));
    ICInvalidateRange(hook->source, sizeof(instr));
    asm volatile("isync" : : : "memory");

    return true;
}

bool applyPointerHook(const tk::PointerHook* hook) {
    const u32 addr = reinterpret_cast<u32>(hook->source);

    LOG("Writing pointer hook: 0x%08X to 0x%08X", addr, hook->target);

    *hook->source = reinterpret_cast<u32>(hook->target);

    DCFlushRange(hook->source, sizeof(void*));

    return true;
}

bool applyPatchHook(const tk::PatchHook* patch) {
    const u32 addr = reinterpret_cast<u32>(patch->addr);
    const u32 totalSize = patch->count * (patch->dataSize / 8);

    LOG("Applying patch at 0x%08X", addr);

    switch (patch->dataSize) {
        default: {
            LOG("Invalid patch unit size %u at addr 0x%08X", patch->dataSize, addr);
            return false;
        }

        case 8: {
            const u8* src = reinterpret_cast<const u8*>(patch->data);
            for (u16 i = 0; i < patch->count; i++)
                reinterpret_cast<u8*>(addr)[i] = src[i];
            break;
        }

        case 16: {
            const u16* src = reinterpret_cast<const u16*>(patch->data);
            for (u16 i = 0; i < patch->count; i++)
                reinterpret_cast<u16*>(addr)[i] = src[i];
            break;
        }

        case 32: {
            const u32* src = reinterpret_cast<const u32*>(patch->data);
            for (u16 i = 0; i < patch->count; i++)
                reinterpret_cast<u32*>(addr)[i] = src[i];
            break;
        }
    }

    DCFlushRange(patch->addr, totalSize);
    ICInvalidateRange(patch->addr, totalSize);
    asm volatile("isync" : : : "memory");

    return true;
}

bool readBranchHook(u32 rpl, void* hookPtr, HookList& list) {
    tk::BranchHook* hook = reinterpret_cast<tk::BranchHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 target = 0;
    s32 err = OSDynLoad_FindExport(rpl, false, hook->target, &target);
    if (err != 0 || target == 0 || target == 0xFFFFFFFF) {
        LOG("Could not find branch hook target: %s for patch at: 0x%08X", hook->target, addr);
        return false;
    }

    hook->target = reinterpret_cast<const char*>(target); //* We are resolving this string early while we still have access to the RPL and reusing the pointer field for the final address
    
    list.add(hook, addr, addr + sizeof(u32));

    return true;
}

bool readPointerHook(u32 rpl, void* hookPtr, HookList& list) {
    tk::PointerHook* hook = reinterpret_cast<tk::PointerHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 target = 0;
    s32 err = OSDynLoad_FindExport(rpl, hook->isData, hook->target, &target);
    if (err != 0 || target == 0 || target == 0xFFFFFFFF) {
        LOG("Could not find pointer hook target: %s for patch at: 0x%08X", hook->target, addr);
        return false;
    }

    hook->target = reinterpret_cast<const char*>(target); //* We are resolving this string early while we still have access to the RPL and reusing the pointer field for the final address

    list.add(hook, addr, addr + sizeof(void*));

    return true;
}

bool readPatchHook(void* hookPtr, HookList& list) {
    const tk::PatchHook* patch = reinterpret_cast<tk::PatchHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(patch->addr);
    const u32 totalSize = patch->count * (patch->dataSize / 8);

    switch (patch->dataSize) {
        default: {
            LOG("Invalid patch unit size %u at addr 0x%08X", patch->dataSize, addr);
            return false;
        }

        case 8:
        case 16:
        case 32:
            break;
    }
    
    list.add(patch, addr, addr + totalSize);

    return true;
}

bool loadRPL(const char* rplName, HookList& hookList, FunctionList& startFuncs) {
    // Acquire RPL
    u32 rpl = 0;
    if (OSDynLoad_Acquire(rplName, &rpl) != 0) {
        LOG("Unable to acquire RPL: %s", rplName);
        return false;
    }
    
    u64 titleID = OSGetTitleID();
    
    using getTitleID_t = u64 (*)();
    getTitleID_t getTitleID = nullptr;
    s32 err = OSDynLoad_FindExport(rpl, 0, "getTitleID", &getTitleID);
    if (err != 0 || getTitleID == nullptr) {
        LOG("Could not find getTitleID, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getTitleID));
        return false;
    }

    u64 rplTitleIDTarget = getTitleID();
    if (titleID != rplTitleIDTarget) {
        u32 rplTitleP1 = (rplTitleIDTarget & 0xFFFFFFFF00000000ULL) >> 32;
        u32 titleIDP1 = (titleID & 0xFFFFFFFF00000000ULL) >> 32;
        
        LOG("RPL %s title ID mismatch, OS: %08X%08X, RPL: %08X%08X", rplName, titleIDP1, titleID & 0xFFFFFFFFULL, rplTitleP1, rplTitleIDTarget & 0xFFFFFFFFULL);
        //return false; // TODO: Figure out why this fails and make it a fatal error
    }
    
    using getModID_t = const char* (*)();
    getModID_t getModID = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getModID", &getModID);
    if (err != 0 || getModID == nullptr) {
        LOG("Could not find getModID, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getModID));
        return false;
    }
    
    const char* const modID = getModID();
    LOG("Mod ID: %s", modID);
    
    // Read start function from RPL
    start_t start = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "__rpl_start", &start);
    if (err != 0 || start == nullptr) {
        LOG("Could not find __rpl_start, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(start));
        return false;
    }
    startFuncs.add(start);
    
    // Find hooks
    GenericHook* hooksBegin = nullptr;
    err = OSDynLoad_FindExport(rpl, 1, "__loaderdata_start", &hooksBegin);
    if (err != 0 || hooksBegin == nullptr) {
        LOG("Could not find data loaderdata_start, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(hooksBegin));
        return false;
    }
    
    GenericHook* hooksEnd = nullptr;
    err = OSDynLoad_FindExport(rpl, 1, "__loaderdata_end", &hooksEnd);
    if (err != 0 || hooksEnd == nullptr) {
        LOG("Could not find data loaderdata_end, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(hooksEnd));
        return false;
    }
    
    LOG("DEBUG: DATA Hooks begin at 0x%08X, end at 0x%08X", reinterpret_cast<u32>(hooksBegin), reinterpret_cast<u32>(hooksEnd));
    if (hooksBegin == nullptr || hooksEnd == nullptr || hooksBegin >= hooksEnd) {
        LOG("Hooks are bad");
        return false;
    }

    // Read hooks
    static_assert(sizeof(GenericHook) == tk::cHookSize, "GenericHook size mismatch");
    u32 hookCount = 0;
    for (GenericHook* hook = hooksBegin; hook != hooksEnd; hook++) {
        switch (hook->magic) {
            case tk::DataMagic::BranchHook: {
                if (!readBranchHook(rpl, hook, hookList))
                    return false;

                break;
            }

            case tk::DataMagic::PointerHook: {
                if (!readPointerHook(rpl, hook, hookList))
                    return false;

                break;
            }

            case tk::DataMagic::PatchHook: {
                if (!readPatchHook(hook, hookList))
                    return false;

                break;
            }
            
            default: {
                LOG("Unknown magic (0x%08X) encountered after %u hooks", hook->magic, hookCount);
                return false;
            }
        }

        hookCount++;
    }

    LOG("Read %u hooks from this RPL: ", hookCount);
    
    return true;
}

} // namespace tk
