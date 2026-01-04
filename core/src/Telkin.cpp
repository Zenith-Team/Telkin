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

#include "PrivateInterface.h"
#include "HookApplicators.h"

#include <semver.h>

#include <vector>
#include <algorithm>
#include <ranges>

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
    struct RequestedDependency {
        const char* requester;
        const char* requestedMod;
        const char* requestedVersion;
    };
    
    std::vector<tk::ModInfo> sAllMods;
    
    bool loadRPL(
        const char* rplName,
        std::vector<HookEntry>& hookList, std::vector<tk::startfunc_t>& startFuncs,
        u32 gameTitleID,
        bool& coreapiEncountered, std::vector<HookEntry>& coreapiHooks, startfunc_t& coreapiStartFunc,
        bool& standardEncountered, bool& coremodEncountered,
        std::vector<HookEntry>& allHooks,
        std::vector<ModInfo>& allMods, std::vector<RequestedDependency>& allDeps
    );
    
    bool applyHooks(const std::vector<HookEntry>& hooks);
    void callFuncs(const std::vector<tk::startfunc_t>& startFuncs, u32 acquireAddr, u32 exportAddr);
    bool validateHooks(std::vector<HookEntry>& hooks);
    bool validateDependencies(const std::vector<RequestedDependency>& deps);
}

extern "C" {
    using funcPtr = void (*)();
    extern funcPtr __init_array_start[], __init_array_end[];
    
    void __rpl_crt() { } // Called by Cafe OS on acquire, don't do anything here
}

extern "C" void MAGIC_CALLBACK(); // just an easy breakpoint site

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
    
    OSReport("Telkin v1.0.0 by Zenith\n");
    tk::sAllMods.emplace_back("Telkin", "1.0.0");

    FSInit();
    OSReport("FS Inited\n");

    FSClient* client = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
    if (client == nullptr) {
        OSReport("Error: Unable to allocate FSClient\n");
        return;
    }
    OSReport("FSClient allocated\n");

    FSCmdBlock* cmd = (FSCmdBlock*)MEMAllocFromDefaultHeap(sizeof(FSCmdBlock));
    if (cmd == nullptr) {
        OSReport("Error: Unable to allocate FSCmdBlock\n");
        MEMFreeToDefaultHeap(client);
        return;
    }
    OSReport("FSCmdBlock allocated\n");

    constexpr int cBufferSize = 0x10000; // I hope rpl.txt won't exceed 10kb ;P
    u8* buffer = (u8*)MEMAllocFromDefaultHeapEx(cBufferSize, FS_IO_BUFFER_ALIGN);
    if (buffer == nullptr) {
        OSReport("Error: Unable to allocate buffer\n");
        MEMFreeToDefaultHeap(client);
        MEMFreeToDefaultHeap(cmd);
        return;
    }
    OSReport("Allocated buffer\n");

    OSBlockSet(buffer, 0, cBufferSize);
    OSReport("OSBlockSet OK\n");

    FSAddClient(client, FS_RET_NO_ERROR);
    OSReport("FSAddClient OK\n");
    FSInitCmdBlock(cmd);
    OSReport("FSInitCmd OK\n");

    char path[FS_MAX_ARGPATH_SIZE];
    //strncpy(path, "/vol/content/rpl.txt", FS_MAX_ARGPATH_SIZE);
    __os_snprintf(path, sizeof(path), "/vol/content/rpl.txt");
    OSReport("strncpy phobia overcame\n");

    FSFileHandle handle;
    FSOpenFile(client, cmd, path, "r", &handle, FS_RET_NO_ERROR);
    OSReport("FSOpenFile OK\n");
    FSReadFile(client, cmd, buffer, 1, cBufferSize, handle, 0, FS_RET_NO_ERROR);
    OSReport("FSReadFile OK\n");

    if (*buffer == 0) {
        OSReport("Error: rpl.txt is empty or non-existent.\n");

        FSCloseFile(client, cmd, handle, FS_RET_NO_ERROR);
        MEMFreeToDefaultHeap(client);
        MEMFreeToDefaultHeap(cmd);
        MEMFreeToDefaultHeap(buffer);

        return;
    } else {
        OSReport("rpl.txt was read\n");
    }
    
    std::vector<tk::HookEntry> stdHooks;
    std::vector<tk::HookEntry> coreapiHooks;
    std::vector<tk::HookEntry> allHooks;
    std::vector<tk::startfunc_t> stdStartFuncs;
    std::vector<tk::RequestedDependency> allDeps;
    tk::startfunc_t coreapiStartFunc = nullptr;
    
    bool success = true;
    bool coreapiEncountered = false;
    bool standardEncountered = false;
    bool coremodEncountered = false;
    u32 gameTitleID = static_cast<u32>(OSGetTitleID());
    
    const char* line = (const char*)buffer;
    for (u32 i = 0; i < cBufferSize; i++) {
        if (buffer[i] == '\n') { // TODO: Support CRLF
            buffer[i] = '\0';
            
            if (!tk::loadRPL(
                line, 
                stdHooks, stdStartFuncs, 
                gameTitleID, 
                coreapiEncountered, coreapiHooks, coreapiStartFunc,
                standardEncountered, coremodEncountered,
                allHooks,
                tk::sAllMods, allDeps
            )) {
                success = false;
                OSReport("RPL failed to load, aborting inject!\n");
                break;
            }
            
            OSReport("Finished loading RPL: %s\n", line);
            
            line = (const char*)(buffer + i + 1);
        }
    }
    
    if (standardEncountered == true && coreapiEncountered == false) {
        OSReport("Attempted to load mods without a CoreAPI. Fix your dependencies. Aborting inject!\n");
        success = false;
    }
    
    if (coreapiEncountered && coremodEncountered) {
        OSReport("Cannot load Core Mods when a CoreAPI is available. Please update your mod or remove the CoreAPI.\n");
        success = false;
    }
    
    if (!success || !tk::validateDependencies(allDeps) || !tk::validateHooks(allHooks)) {
        FSCloseFile(client, cmd, handle, FS_RET_NO_ERROR);
        MEMFreeToDefaultHeap(client);
        MEMFreeToDefaultHeap(cmd);
        MEMFreeToDefaultHeap(buffer);
        
        OSReport("Something went wrong. You can ask for help in our Discord server: https://go.nsmbu.net/discord or email: contact@nsmbu.net\n");
        
        return; // no changes to game
    }
    
    // here's the magic:
    MAGIC_CALLBACK();
    tk::applyHooks(coreapiHooks);
    tk::applyHooks(stdHooks);
    coreapiStartFunc(
        OS_SPECIFICS->addr_OSDynLoad_Acquire,
        OS_SPECIFICS->addr_OSDynLoad_FindExport
    );
    tk::callFuncs(
        stdStartFuncs,
        OS_SPECIFICS->addr_OSDynLoad_Acquire,
        OS_SPECIFICS->addr_OSDynLoad_FindExport
    );
    
    FSCloseFile(client, cmd, handle, FS_RET_NO_ERROR);
    OSReport("FSCloseFile OK\n");
    MEMFreeToDefaultHeap(client);
    MEMFreeToDefaultHeap(cmd);
    MEMFreeToDefaultHeap(buffer);
    OSReport("MEMFrees OK\n");

    OSReport("Telkin is finished loading mods. Enjoy the game!\n");
    
    return;
}

namespace tk {

const std::span<ModInfo> getMods() {
    return sAllMods;
}

bool loadRPL(
    const char* rplName,
    std::vector<HookEntry>& hookList, std::vector<tk::startfunc_t>& startFuncs,
    u32 gameTitleID,
    bool& coreapiEncountered, std::vector<HookEntry>& coreapiHooks, startfunc_t& coreapiStartFunc,
    bool& standardEncountered, bool& coremodEncountered,
    std::vector<HookEntry>& allHooks,
    std::vector<ModInfo>& allMods, std::vector<RequestedDependency>& allDeps
) {
    // Acquire RPL
    u32 rpl = 0;
    if (OSDynLoad_Acquire(rplName, &rpl) != 0) {
        OSReport("Unable to acquire RPL: %s\n", rplName);
        return false;
    }
    
    getTitleID_t getTitleID = nullptr;
    s32 err = OSDynLoad_FindExport(rpl, 0, "getTitleID", &getTitleID);
    if (err != 0 || getTitleID == nullptr) {
        OSReport("Could not find getTitleID, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(getTitleID));
        return false;
    }

    u32 rplTitleIDTarget = static_cast<u32>(getTitleID());
    if (gameTitleID != rplTitleIDTarget) {
        OSReport("RPL %s title ID mismatch, OS: %08X, RPL: %08X\n", rplName, gameTitleID, rplTitleIDTarget);
        return false;
    }
    
    getModID_t getModID = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getModID", &getModID);
    if (err != 0 || getModID == nullptr) {
        OSReport("Could not find getModID, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(getModID));
        return false;
    }
    
    const char* const modID = getModID();
    OSReport("Mod ID: %s\n", modID);
    
    getModID_t getVersion = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getVersion", &getVersion);
    if (err != 0 || getVersion == nullptr) {
        OSReport("Could not find getVersion, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(getVersion));
        return false;
    }
    
    const char* const modVersion = getVersion();
    
    allMods.emplace_back(modID, modVersion);
    
    // Check type
    getModuleType_t getModuleType = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getModuleType", &getModuleType);
    if (err != 0 || getModID == nullptr) {
        OSReport("Could not find getModuleType, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(getModuleType));
        return false;
    }
    
    // Read dependencies
    getDependencyManifest_t getDependencyManifest = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getDependencyManifest", &getDependencyManifest);
    if (err != 0 || getDependencyManifest == nullptr) {
        OSReport("Could not find getDependencyManifest, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(getDependencyManifest));
        return false;
    }
    
    const u8* dependencyManifest = getDependencyManifest();
    
    u32 dependencyCount = *(u32*)dependencyManifest;
    dependencyManifest += sizeof(u32);
    OSReport("Found %u dependencies\n", dependencyCount);
    for (u32 i = 0; i < dependencyCount; i++) {
        u32 nameLen = 0;
        for (const u8* c = dependencyManifest; *c != 0x00; c++) {
            nameLen++;
        }
        
        const u8* name = dependencyManifest;
        const u8* version = dependencyManifest + nameLen + 1;
        
        OSReport("Dependency: [%s, %s]\n", name, version);
        allDeps.emplace_back((const char*)modID, (const char*)name, (const char*)version);
        
        u32 versionLen = 0;
        for (const u8* c = version; *c != 0x00; c++) {
            versionLen++;
        }
        
        dependencyManifest += nameLen + 1 + versionLen + 1;
    }
    
    std::vector<HookEntry>* outputHookList = &hookList;
    
    ModuleType moduleType = getModuleType();
    switch (moduleType) {
        case tk::ModuleType::CoreAPI: {
            if (coreapiEncountered) {
                OSReport("Cannot load multiple CoreAPI modules simultaneously!\n");
                return false;
            }
            
            coreapiEncountered = true;
            outputHookList = &coreapiHooks;
            
            // Read start function from RPL
            tk::startfunc_t start = nullptr;
            err = OSDynLoad_FindExport(rpl, 0, "__rpl_start", &start);
            if (err != 0 || start == nullptr) {
                OSReport("Could not find __rpl_start, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(start));
                return false;
            }
            coreapiStartFunc = start;
            
            break;
        }
        
        case tk::ModuleType::CoreMod: {
            if (coreapiEncountered) {
                OSReport("Cannot load Core Mods when a CoreAPI is available. Please update your mod or remove the CoreAPI.\n");
                return false;
            }
            
            coremodEncountered = true;
            
            // Read start function from RPL
            tk::startfunc_t start = nullptr;
            err = OSDynLoad_FindExport(rpl, 0, "__rpl_start", &start);
            if (err != 0 || start == nullptr) {
                OSReport("Could not find __rpl_start, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(start));
                return false;
            }
            startFuncs.emplace_back(start);
            
            break;
        };
        
        case tk::ModuleType::Standard: {
            // Read start function from RPL
            tk::startfunc_t start = nullptr;
            err = OSDynLoad_FindExport(rpl, 0, "__rpl_start", &start);
            if (err != 0 || start == nullptr) {
                OSReport("Could not find __rpl_start, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(start));
                return false;
            }
            startFuncs.emplace_back(start);
            
            standardEncountered = true;
            
            break;
        }
        
        case tk::ModuleType::Special: {
            OSReport("WARNING: Why are you using ModuleType Special? It does nothing for you.\n");
            break;
        }
    }
    
    // Find hooks
    GenericHook* hooksBegin = nullptr;
    err = OSDynLoad_FindExport(rpl, 1, "__loaderdata_start", &hooksBegin);
    if (err != 0 || hooksBegin == nullptr) {
        OSReport("Could not find data loaderdata_start, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(hooksBegin));
        OSReport("Assuming no hooks.\n");
        return true;
    }
    
    GenericHook* hooksEnd = nullptr;
    err = OSDynLoad_FindExport(rpl, 1, "__loaderdata_end", &hooksEnd);
    if (err != 0 || hooksEnd == nullptr) {
        OSReport("Could not find data loaderdata_end, err = 0x%08X, ptr = 0x%08X\n", err, reinterpret_cast<u32>(hooksEnd));
    }
    
    OSReport("DEBUG: DATA Hooks begin at 0x%08X, end at 0x%08X\n", reinterpret_cast<u32>(hooksBegin), reinterpret_cast<u32>(hooksEnd));
    if (hooksBegin == nullptr || hooksEnd == nullptr || hooksBegin >= hooksEnd) {
        OSReport("Hooks are bad");
        return false;
    }

    // Read hooks
    static_assert(sizeof(GenericHook) == tk::cHookSize, "GenericHook size mismatch");
    u32 hookCount = 0;
    for (GenericHook* hook = hooksBegin; hook != hooksEnd; hook++) {
        switch (hook->magic) {
            case tk::DataMagic::BranchHook: {
                if (!tk::readBranchHook(rpl, hook, *outputHookList, allHooks))
                    return false;

                break;
            }

            case tk::DataMagic::PointerHook: {
                if (!tk::readPointerHook(rpl, hook, *outputHookList, allHooks))
                    return false;

                break;
            }

            case tk::DataMagic::PatchHook: {
                if (!tk::readPatchHook(hook, *outputHookList, allHooks))
                    return false;

                break;
            }
            
            default: {
                OSReport("Unknown magic (0x%08X) encountered after %u hooks\n", hook->magic, hookCount);
                return false;
            }
        }

        hookCount++;
    }

    OSReport("Finished reading %u hooks from this RPL: %s\n", hookCount, rplName);
    
    return true;
}

bool applyHooks(const std::vector<HookEntry>& hooks) {
    for (const HookEntry& entry : hooks) {
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

bool validateHooks(std::vector<HookEntry>& hooks) {
    OSReport("Validating hooks...\n");
    
    if (hooks.size() <= 1) {
        OSReport("Only %u hooks present, assuming no conflicts.\n", hooks.size());
        return true;
    }
    
    std::sort(hooks.begin(), hooks.end(), [](const HookEntry& lhs, const HookEntry& rhs) {
        return lhs.startAddr < rhs.startAddr;
    });
    
    for (s32 i = 1; i < hooks.size(); i++) {
        if (hooks.data()[i].startAddr < hooks.data()[i - 1].endAddr) {
            // TODO: Better diagnostic here with mod blame and addrs/types
            OSReport("MOD INCOMPATIBILITY: Overlapping hooks found!\n");
            return false;
        }
    }
    
    OSReport("No hook conflicts found :)\n");
    return true;
}

static s32 check_manifest_dependency(const char* manifest_req, const char* concrete_ver_str) {
    if (!manifest_req || !concrete_ver_str) {
        return -1;
    }

    semver_t manifest_version = {};
    semver_t concrete_version = {};
    char op[5] = {0};
    const char* p = manifest_req;
    s32 op_idx = 0;

    while (*p && isspace((unsigned char)*p)) {
        p++;
    }

    while (*p && strchr(">=<~^!", *p)) {
        if (op_idx < 4) {
            op[op_idx++] = *p;
            p++;
        } else {
            return -1;
        }
    }

    while (*p && isspace((unsigned char)*p)) {
        p++;
    }

    if (op_idx == 0) {
        strcpy(op, "=");
    } else {
        const char* valid_ops[] = {">=", "<=", ">", "<", "=", "~", "^", "!=", NULL};
        s32 valid = 0;
        for (s32 i = 0; valid_ops[i] != NULL; i++) {
            if (strcmp(op, valid_ops[i]) == 0) {
                valid = 1;
                break;
            }
        }
        if (!valid) {
            return -1;
        }
    }

    if (*p == 'v' || *p == 'V') {
        p++;
    }

    while (*p && isspace((unsigned char)*p)) {
        p++;
    }

    if (strcmp(p, "*") == 0 || strcmp(p, "x") == 0 || strcmp(p, "X") == 0) {
        if (op_idx > 0 && strcmp(op, "=") != 0) {
            return -1;
        }
        return 1;
    }

    const char* wildcard_pos = strpbrk(p, "*xX");
    if (wildcard_pos) {
        if (op_idx > 0 && strcmp(op, "=") != 0) {
            return -1;
        }

        char prefix[256];
        size_t prefix_len = wildcard_pos - p;
    
        while (prefix_len > 0 && p[prefix_len - 1] == '.') {
            prefix_len--;
        }
        
        if (prefix_len >= sizeof(prefix)) {
            return -1;
        }
        
        memcpy(prefix, p, prefix_len);
        prefix[prefix_len] = '\0';
        
        const char* concrete_p = concrete_ver_str;
        while (*concrete_p && isspace((unsigned char)*concrete_p)) {
            concrete_p++;
        }
        if (*concrete_p == 'v' || *concrete_p == 'V') {
            concrete_p++;
        }
        while (*concrete_p && isspace((unsigned char)*concrete_p)) {
            concrete_p++;
        }
        
        if (prefix_len == 0) {
            return 1;
        }
        
        if (strncmp(concrete_p, prefix, prefix_len) == 0) {
            char next = concrete_p[prefix_len];
            if (next == '.' || next == '\0') {
                return 1;
            }
        }
        
        return 0;
    }

    if (!*p) {
        return -1;
    }
    
    if (!isdigit((unsigned char)*p)) {
        return -1;
    }

    if (semver_parse(p, &manifest_version) < 0) {
        return -1;
    }

    const char* concrete_p = concrete_ver_str;
    while (*concrete_p && isspace((unsigned char)*concrete_p)) {
        concrete_p++;
    }
    if (*concrete_p == 'v' || *concrete_p == 'V') {
        concrete_p++;
    }
    while (*concrete_p && isspace((unsigned char)*concrete_p)) {
        concrete_p++;
    }

    if (semver_parse(concrete_p, &concrete_version) < 0) {
        semver_free(&manifest_version);
        return -2;
    }

    s32 result = semver_satisfies(concrete_version, manifest_version, op);

    semver_free(&manifest_version);
    semver_free(&concrete_version);

    return result == 1;
}

static u8 lower(u8 c) {
    if (c >= 'A' && c <= 'Z') {
        return c + ('a' - 'A');
    }
    return c;
}

static int caselesscmp(const char* s1, const char* s2) {
    const u8* p1 = (const u8*)s1;
    const u8* p2 = (const u8*)s2;

    while (*p1 && (lower(*p1) == lower(*p2))) {
        p1++;
        p2++;
    }

    return lower(*p1) - lower(*p2);
}

bool validateDependencies(const std::vector<RequestedDependency>& deps) { // TODO: We can optimize this, but is it necessary or worth it? (Evaluate memory/speed tradeoff)
    const std::span<ModInfo> mods = getMods();
    
    OSReport("--BEGIN LOADED MODS--\n");
    for (const ModInfo& mod : mods) {
        OSReport("Mod: %s, %s\n", mod.id, mod.version);
    }
    OSReport("--END LOADED MODS--\n");
    
    for (const auto& [requester, requestedMod, requestedVersion] : deps) {
        auto it = std::ranges::find_if(mods, [requestedMod](const ModInfo& mod){
            return caselesscmp(mod.id, requestedMod) == 0;
        });
        
        if (it == mods.end()) {
            OSReport("Missing Dependency: '%s' (requested by %s)\n", requestedMod, requester);
            return false;
        }
        
        // check for semver equality
        s32 result = check_manifest_dependency(requestedVersion, it->version);
        if (result == -1) {
            OSReport("Invalid version range '%s' for mod '%s' requested by '%s'\n", requestedVersion, requestedMod, requester);
            return false;
        } else if (result == -2) {
            OSReport("Installed mod '%s' has invalid version string: '%s'\n", it->id, it->version);
            return false;
        } else if (result == 0) {
            OSReport("Version Mismatch for '%s': Needed %s, found %s\n", requestedMod, requestedVersion, it->version);
            return false;
        }
    }
    
    OSReport("Dependencies validated.\n");
    
    return true;
}

void callFuncs(const std::vector<tk::startfunc_t>& startFuncs, u32 acquireAddr, u32 exportAddr) {
    for (const tk::startfunc_t func : startFuncs) {
        func(acquireAddr, exportAddr);
    }
}

} // namespace tk
