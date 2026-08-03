#include <telkin/Telkin.h>

#include <dynamic_libs/os_functions.h>
#include <dynamic_libs/acp_functions.h>
#include <dynamic_libs/aoc_functions.h>
#include <dynamic_libs/ax_functions.h>
#include <dynamic_libs/fs_functions.h>
#include <dynamic_libs/gx2_functions.h>
#include <dynamic_libs/h264_functions.h>
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
    bool sIsCemu = false;

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
    bool validateHooks(std::vector<HookEntry>& hooks);
    bool validateDependencies(const std::vector<RequestedDependency>& deps);
}

extern "C" {
    using funcPtr = void (*)();
    extern funcPtr __init_array_start[], __init_array_end[];

    int __rpl_crt() { return 0; } // Called by Cafe OS on acquire, don't do anything here
}

// Ensure .loaderdata isn't empty
tk::NullHook _tHook_NullInit __attribute__((section(".loaderdata"))) = tk::NullHook(tk::DataMagic::BranchHook);

// Ensure .init_array isn't empty
#pragma clang optimize off
struct NullCtor {
    [[clang::noinline, gnu::used]]
    NullCtor() : x(0) {
        asm volatile("nop");
    }
    ~NullCtor() = default;
    int x;
};

[[gnu::used]]
NullCtor _tHook_NullCtor;
#pragma clang optimize on

template <typename T>
class UniquePtrMEM {
public:
    UniquePtrMEM(size_t n)
        : mPtr((T*)MEMAllocFromDefaultHeap(n))
    { }
    
    ~UniquePtrMEM() {
        MEMFreeToDefaultHeap(mPtr);
    }
    
    T* get() const { return mPtr; }
    
private:
    T* mPtr;
};

tk::writefunc_t tk::privilegedWrite = nullptr;

void directWrite(const void* dst, const void* src, u32 len) {
    OSBlockMove((void*)dst, src, len, 1);
}

static int caselesscmp(const char* s1, const char* s2); // forward decl

extern "C" void init(u32 acquireAddr, u32 exportAddr, tk::writefunc_t writeFunc) {
    static bool initialized = false;
    if (initialized)
      return;
    initialized = true;
    
    if (writeFunc != nullptr) {
        tk::privilegedWrite = writeFunc;
    } else {
        tk::privilegedWrite = &directWrite;
        tk::sIsCemu = true;
    }

    OS_SPECIFICS->addr_OSDynLoad_Acquire = acquireAddr;
    OS_SPECIFICS->addr_OSDynLoad_FindExport = exportAddr;

    for (funcPtr* p = __init_array_start; p != __init_array_end; p++) {
        (*p)();
    }

    InitOSFunctionPointers();
    InitACPFunctionPointers();
    InitAocFunctionPointers();
    InitAXFunctionPointers();
    InitFSFunctionPointers();
    InitGX2FunctionPointers();
    InitH264FunctionPointers();
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

    tk::println("Telkin v" MOD_VERSION " by Zenith");
    tk::sAllMods.emplace_back("telkin", MOD_VERSION);

    FSInit();
    tk::println("FS Inited");
    
    std::vector<tk::HookEntry> stdHooks;
    std::vector<tk::HookEntry> coreapiHooks;
    std::vector<tk::HookEntry> allHooks;
    std::vector<tk::startfunc_t> stdStartFuncs;
    std::vector<tk::RequestedDependency> allDeps;
    tk::startfunc_t coreapiStartFunc = nullptr;

    {
        UniquePtrMEM<FSClient> client = sizeof(FSClient);
        if (client.get() == nullptr) {
            tk::println("Error: Unable to allocate FSClient");
            return;
        }
        tk::println("FSClient allocated");
    
        UniquePtrMEM<FSCmdBlock> cmd = sizeof(FSCmdBlock);
        if (cmd.get() == nullptr) {
            tk::println("Error: Unable to allocate FSCmdBlock");
            return;
        }
        tk::println("FSCmdBlock allocated");
    
        FSAddClient(client.get(), FS_RET_NO_ERROR);
        tk::println("FSAddClient OK");
        FSInitCmdBlock(cmd.get());
        tk::println("FSInitCmd OK");
    
        const u64 titleID = OSGetTitleID();
        u32 titleID_top = (titleID >> 32) & 0xFFFFFFFFU;
        u32 titleID_bot = titleID & 0xFFFFFFFFU;
        if (!tk::isCemu()) {
            titleID_top |= 0xC0000000U; //* tag for console
        }
        tk::println("Identified title id: %08X%08X", titleID_top, titleID_bot);
    
        char coreDirPath[FS_MAX_ARGPATH_SIZE];
        __os_snprintf(coreDirPath, sizeof(coreDirPath), "/vol/content/telkin/%08X%08X/core/", titleID_top, titleID_bot);
        FSDirHandle coreDir;
        if (FSOpenDir(client.get(), cmd.get(), coreDirPath, &coreDir, FS_RET_ALL_ERROR) != FS_STATUS_OK) {
            tk::println("Couldn't find coremods path. Gracefully returning...");
            return;
        }
        tk::println("FSOpenDir1 %s OK", coreDirPath);
        
        char modsDirPath[FS_MAX_ARGPATH_SIZE];
        __os_snprintf(modsDirPath, sizeof(modsDirPath), "/vol/content/telkin/%08X%08X/mods/", titleID_top, titleID_bot);
        FSDirHandle modsDir;
        const bool hasStandardMods = FSOpenDir(client.get(), cmd.get(), modsDirPath, &modsDir, FS_RET_ALL_ERROR) == FS_STATUS_OK;
        tk::println("FSOpenDir2 %s OK: %s", modsDirPath, hasStandardMods ? "Standard mods found" : "No standard mods present");
        
        // Read & load
        bool success = true;
        bool coreapiEncountered = false;
        bool standardEncountered = false;
        bool coremodEncountered = false;
        u32 gameTitleID = static_cast<u32>(titleID);
        FSDirEntry directoryEntry;
        // Pass 1: Core mods
        while (FSReadDir(client.get(), cmd.get(), coreDir, &directoryEntry, FS_RET_NO_ERROR) == FS_STATUS_OK) {
            tk::println("Loading %s.rpl", directoryEntry.name);
            
            success = tk::loadRPL(
                directoryEntry.name,
                stdHooks, stdStartFuncs,
                gameTitleID,
                coreapiEncountered, coreapiHooks, coreapiStartFunc,
                standardEncountered, coremodEncountered,
                allHooks,
                tk::sAllMods, allDeps
            );
            
            if (!success) {
                tk::fatal("RPL failed to load, aborting inject!");
                break;
            }
        }
        // Pass 2: Standard mods
        while (hasStandardMods && FSReadDir(client.get(), cmd.get(), modsDir, &directoryEntry, FS_RET_NO_ERROR) == FS_STATUS_OK) {
            tk::println("Loading %s.rpl", directoryEntry.name);
            
            success = tk::loadRPL(
                directoryEntry.name,
                stdHooks, stdStartFuncs,
                gameTitleID,
                coreapiEncountered, coreapiHooks, coreapiStartFunc,
                standardEncountered, coremodEncountered,
                allHooks,
                tk::sAllMods, allDeps
            );
            
            if (!success) {
                tk::fatal("RPL failed to load, aborting inject!");
                break;
            }
        }
    
        if (standardEncountered == true && coreapiEncountered == false) {
            tk::fatal("Attempted to load mods without a CoreAPI. Fix your dependencies. Aborting inject!");
            success = false;
        }
    
        if (coreapiEncountered && coremodEncountered) {
            tk::fatal("Cannot load Core Mods when a CoreAPI is available. Please update your mod or remove the CoreAPI.");
            success = false;
        }
    
        if (!success || !tk::validateDependencies(allDeps) || !tk::validateHooks(allHooks)) {
            tk::fatal("Something went wrong. You can ask for help in our Discord server: https://go.nsmbu.net/discord or email: contact@nsmbu.net");
    
            return; // no changes to game
        }
    }

    // here's the magic:
    if (!(tk::applyHooks(coreapiHooks) && tk::applyHooks(stdHooks))) {
        tk::fatal("Something went wrong. You can ask for help in our Discord server: https://go.nsmbu.net/discord or email: contact@nsmbu.net");
    }
    if (coreapiStartFunc) {
        coreapiStartFunc(
            OS_SPECIFICS->addr_OSDynLoad_Acquire,
            OS_SPECIFICS->addr_OSDynLoad_FindExport
        );
    }
    for (const tk::startfunc_t func : stdStartFuncs) {
        func(
            OS_SPECIFICS->addr_OSDynLoad_Acquire,
            OS_SPECIFICS->addr_OSDynLoad_FindExport
        );
    }

    tk::println("Telkin is finished loading mods. Enjoy the game!");

    return;
}

const std::span<tk::ModInfo> tk::getMods() {
    return sAllMods;
}

bool tk::isModLoaded(const char* id) {
    const std::span<ModInfo> mods = tk::getMods();
    std::string_view target{id};
    
    return std::ranges::any_of(mods, [target](const ModInfo& mod) {
        return mod.id == target; 
    });
}

bool tk::isCemu() {
    return tk::sIsCemu;
}

bool tk::loadRPL(
    const char* rplName,
    std::vector<HookEntry>& hookList, std::vector<startfunc_t>& startFuncs,
    u32 gameTitleID,
    bool& coreapiEncountered, std::vector<HookEntry>& coreapiHooks, startfunc_t& coreapiStartFunc,
    bool& standardEncountered, bool& coremodEncountered,
    std::vector<HookEntry>& allHooks,
    std::vector<ModInfo>& allMods, std::vector<RequestedDependency>& allDeps
) {
    // Acquire RPL
    u32 rpl = 0;
    if (tk::isCemu()) {
        if (OSDynLoad_Acquire(rplName, &rpl) != 0) {
            tk::fatal("Unable to acquire RPL: %s", rplName);
            return false;
        }
    } else {
        // console
        static char rplPath[64]; // the "name" here can only be 64 chars wide in the loader
        __os_snprintf(rplPath, sizeof(rplPath), "~/telkin/%016llX/code/%s", OSGetTitleID(), rplName);
        if (OSDynLoad_Acquire(rplPath, &rpl) != 0) {
            tk::fatal("Unable to acquire RPL: %s", rplPath);
            return false;
        }
    }

    getTitleID_t getTitleID = nullptr;
    s32 err = OSDynLoad_FindExport(rpl, 0, "getTitleID", &getTitleID);
    if (err != 0 || getTitleID == nullptr) {
        tk::fatal("Could not find getTitleID, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getTitleID));
        return false;
    }

    u32 rplTitleIDTarget = static_cast<u32>(getTitleID());
    if (gameTitleID != rplTitleIDTarget) {
        tk::fatal("RPL %s title ID mismatch, OS: %08X, RPL: %08X", rplName, gameTitleID, rplTitleIDTarget);
        return false;
    }

    getModID_t getModID = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getModID", &getModID);
    if (err != 0 || getModID == nullptr) {
        tk::fatal("Could not find getModID, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getModID));
        return false;
    }

    const char* const modID = getModID();
    tk::println("Mod ID: %s", modID);
    
    for (const auto& otherMod : allMods) {
        if (caselesscmp(otherMod.id, modID) == 0) {
            tk::fatal("Duplicate mods loaded: %s", modID);
            return false;
        }
    }

    getModID_t getVersion = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getVersion", &getVersion);
    if (err != 0 || getVersion == nullptr) {
        tk::fatal("Could not find getVersion, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getVersion));
        return false;
    }

    const char* const modVersion = getVersion();

    allMods.emplace_back(modID, modVersion);

    // Check type
    getModuleType_t getModuleType = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getModuleType", &getModuleType);
    if (err != 0 || getModID == nullptr) {
        tk::fatal("Could not find getModuleType, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getModuleType));
        return false;
    }

    // Read dependencies
    getDependencyManifest_t getDependencyManifest = nullptr;
    err = OSDynLoad_FindExport(rpl, 0, "getDependencyManifest", &getDependencyManifest);
    if (err != 0 || getDependencyManifest == nullptr) {
        tk::fatal("Could not find getDependencyManifest, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(getDependencyManifest));
        return false;
    }

    const u8* dependencyManifest = getDependencyManifest();

    u32 dependencyCount = *(u32*)dependencyManifest;
    dependencyManifest += sizeof(u32);
    tk::println("Found %u dependencies", dependencyCount);
    for (u32 i = 0; i < dependencyCount; i++) {
        u32 nameLen = 0;
        for (const u8* c = dependencyManifest; *c != 0x00; c++) {
            nameLen++;
        }

        const u8* name = dependencyManifest;
        const u8* version = dependencyManifest + nameLen + 1;

        tk::println("Dependency: [%s, %s]", name, version);
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
                tk::fatal("Cannot load multiple CoreAPI modules simultaneously!");
                return false;
            }

            coreapiEncountered = true;
            outputHookList = &coreapiHooks;

            // Read start function from RPL
            tk::startfunc_t start = nullptr;
            err = OSDynLoad_FindExport(rpl, 0, "__rpl_start", &start);
            if (err != 0 || start == nullptr) {
                tk::fatal("Could not find __rpl_start, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(start));
                return false;
            }
            coreapiStartFunc = start;

            break;
        }

        case tk::ModuleType::CoreMod: {
            if (coreapiEncountered) {
                tk::fatal("Cannot load Core Mods when a CoreAPI is available. Please update your mod or remove the CoreAPI.");
                return false;
            }

            coremodEncountered = true;

            // Read start function from RPL
            tk::startfunc_t start = nullptr;
            err = OSDynLoad_FindExport(rpl, 0, "__rpl_start", &start);
            if (err != 0 || start == nullptr) {
                tk::fatal("Could not find __rpl_start, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(start));
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
                tk::fatal("Could not find __rpl_start, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(start));
                return false;
            }
            startFuncs.emplace_back(start);

            standardEncountered = true;

            break;
        }

        case tk::ModuleType::Special: {
            tk::fatal("Why are you using ModuleType Special? It does nothing for you.");
            break;
        }
    }

    // Find hooks
    GenericHook* hooksBegin = nullptr;
    err = OSDynLoad_FindExport(rpl, 1, "__loaderdata_start", &hooksBegin);
    if (err != 0 || hooksBegin == nullptr) {
        tk::println("Could not find data loaderdata_start, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(hooksBegin));
        tk::println("Assuming no hooks.");
        return true;
    }

    GenericHook* hooksEnd = nullptr;
    err = OSDynLoad_FindExport(rpl, 1, "__loaderdata_end", &hooksEnd);
    if (err != 0 || hooksEnd == nullptr) {
        tk::println("Could not find data loaderdata_end, err = 0x%08X, ptr = 0x%08X", err, reinterpret_cast<u32>(hooksEnd));
    }

    tk::println("DEBUG: DATA Hooks begin at 0x%08X, end at 0x%08X", reinterpret_cast<u32>(hooksBegin), reinterpret_cast<u32>(hooksEnd));
    if (hooksBegin == nullptr || hooksEnd == nullptr || hooksBegin >= hooksEnd) {
        tk::fatal("Hooks are bad");
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
            
            case tk::DataMagic::NullHook: {
                break;
            }

            default: {
                tk::fatal("Unknown magic (0x%08X) encountered after %u hooks", hook->magic, hookCount);
                return false;
            }
        }

        hookCount++;
    }

    tk::println("Finished reading %u hooks from this RPL: %s", hookCount, rplName);

    return true;
}

bool tk::applyHooks(const std::vector<HookEntry>& hooks) {
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

bool tk::validateHooks(std::vector<HookEntry>& hooks) {
    tk::println("Validating hooks...");

    if (hooks.size() <= 1) {
        tk::println("Only %u hooks present, assuming no conflicts.", hooks.size());
        return true;
    }

    std::sort(hooks.begin(), hooks.end(), [](const HookEntry& lhs, const HookEntry& rhs) {
        return lhs.startAddr < rhs.startAddr;
    });

    for (size_t i = 1; i < hooks.size(); i++) {
        if (hooks.data()[i].startAddr < hooks.data()[i - 1].endAddr) {
            // TODO: Better diagnostic here with mod blame and addrs/types
            tk::fatal("MOD INCOMPATIBILITY: Overlapping hooks found!");
            return false;
        }
    }

    tk::println("No hook conflicts found :)");
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

bool tk::validateDependencies(const std::vector<RequestedDependency>& deps) { // TODO: We can optimize this, but is it necessary or worth it? (Evaluate memory/speed tradeoff)
    const std::span<ModInfo> mods = getMods();

    tk::println("--BEGIN LOADED MODS--");
    for (const ModInfo& mod : mods) {
        tk::println("Mod: %s, %s", mod.id, mod.version);
    }
    tk::println("--END LOADED MODS--");

    for (const auto& [requester, requestedMod, requestedVersion] : deps) {
        auto it = std::ranges::find_if(mods, [requestedMod](const ModInfo& mod){
            return caselesscmp(mod.id, requestedMod) == 0;
        });

        if (it == mods.end()) {
            tk::fatal("Missing Dependency: '%s' (requested by %s)", requestedMod, requester);
            return false;
        }

        // check for semver equality
        s32 result = check_manifest_dependency(requestedVersion, it->version);
        if (result == -1) {
            tk::fatal("Invalid version range '%s' for mod '%s' requested by '%s'", requestedVersion, requestedMod, requester);
            return false;
        } else if (result == -2) {
            tk::fatal("Installed mod '%s' has invalid version string: '%s'", it->id, it->version);
            return false;
        } else if (result == 0) {
            tk::fatal("Version Mismatch for '%s': Needed %s, found %s", requestedMod, requestedVersion, it->version);
            return false;
        }
    }

    tk::println("Dependencies validated.");

    return true;
}
