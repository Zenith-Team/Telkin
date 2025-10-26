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

#include "Debug.h"
#include "Lists.h"


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

namespace tk {
    bool loadRPL(const char* rplName, tk::HookList& hookList, tk::FunctionList& startFuncs, u32 gameTitleID);
    
    char logMsg[tk::cLogBufferSize];
}

extern "C" {
    using funcPtr = void (*)();
    extern funcPtr __init_array_start[], __init_array_end[];
    
    void __rpl_crt() { } // Called by Cafe OS on acquire, don't do anything here
}

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
        tk::HookList hooks;
        tk::FunctionList startFuncs;
        
        bool success = true;
        u32 gameTitleID = static_cast<u32>(OSGetTitleID());
        
        char* line = (char*)buffer;
        for (u32 i = 0; i < cBufferSize; i++) {
            if (buffer[i] == '\n') { // TODO: Support CRLF
                buffer[i] = '\0';
                
                if (!tk::loadRPL(line, hooks, startFuncs, gameTitleID)) {
                    success = false;
                    LOG("RPL %s failed to load, aborting inject!");
                    break;
                }
                
                LOG("Finished loading RPL: %s", line);

                line = (char*)(buffer + i + 1);
            }
        }
        
        if (!success || !hooks.validateRanges()) {
            FSCloseFile(client, cmd, handle, FS_RET_NO_ERROR);
            MEMFreeToDefaultHeap(client);
            MEMFreeToDefaultHeap(cmd);
            MEMFreeToDefaultHeap(buffer);
            
            return; // no changes to game
        }
        
        // here's the magic:
        hooks.applyAll();
        startFuncs.callAll(
            OS_SPECIFICS->addr_OSDynLoad_Acquire,
            OS_SPECIFICS->addr_OSDynLoad_FindExport
        );
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

bool loadRPL(const char* rplName, HookList& hookList, FunctionList& startFuncs, u32 gameTitleID) {
    // Acquire RPL
    u32 rpl = 0;
    if (OSDynLoad_Acquire(rplName, &rpl) != 0) {
        LOG("Unable to acquire RPL: %s", rplName);
        return false;
    }
    
    using getTitleID_t = u64 (*)();
    getTitleID_t getTitleID = nullptr;
    s32 err = OSDynLoad_FindExport(rpl, 0, "getTitleID", &getTitleID);
    if (err != 0 || getTitleID == nullptr) {
        LOG("Could not find getTitleID, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getTitleID));
        return false;
    }

    u32 rplTitleIDTarget = static_cast<u32>(getTitleID());
    if (gameTitleID != rplTitleIDTarget) {
        LOG("RPL %s title ID mismatch, OS: %08X, RPL: %08X", rplName, gameTitleID, rplTitleIDTarget);
        return false;
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
    tk::startfunc_t start = nullptr;
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
