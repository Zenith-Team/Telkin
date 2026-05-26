#include <string.h>
#include <format>

#include <wups.h>
#include <whb/log.h>
#include <whb/log_module.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>

#include <content_redirection/redirection.h>
#include <kernel/kernel.h>

#include <coreinit/cache.h>
#include <coreinit/dynload.h>
#include <coreinit/internal.h>
#include <coreinit/title.h>
#include <coreinit/memorymap.h>

WUPS_PLUGIN_NAME("Telkin RPL Loader");
WUPS_PLUGIN_DESCRIPTION("A dynamic Wii U mod loader.");
WUPS_PLUGIN_VERSION("1.0.1");
WUPS_PLUGIN_AUTHOR("techmuse, Luminyx");
WUPS_PLUGIN_LICENSE("MPL-2.0");

WUPS_USE_WUT_DEVOPTAB();

static bool TelkinInitialized = 0;
OSDynLoad_Module TelkinRPLHandle = nullptr;
static CRLayerHandle contentLayerHandle;
static CRLayerHandle aocLayerHandle;

void KernWriteWrapper(uint32_t dst, uint32_t src, uint32_t len) {
    KernelCopyData(OSEffectiveToPhysical(dst), OSEffectiveToPhysical(src), len);
}

void TelkinBootstrap() {
    WHBLogPrintf("In the Telkin bootstrap!\n");
    if (TelkinInitialized)
        return;

    using Telkin_writeFunc_t = void (*)(uint32_t, uint32_t, uint32_t);
    using OSDynLoad_Acquire_t = OSDynLoad_Error (*)(char const *, OSDynLoad_Module *);
    using OSDynLoad_Export_t = OSDynLoad_Error (*)(OSDynLoad_Module, OSDynLoad_ExportType, const char *, void **);

    using Telkin_init_t = void (*)(OSDynLoad_Acquire_t acquireAddr, OSDynLoad_Export_t exportAddr, Telkin_writeFunc_t writeFunc);
    Telkin_init_t Telkin_init;

    char cpy[64] = { };
    __os_snprintf(cpy, sizeof(cpy), "~/telkin/%016llX/code/Telkin.rpl", OSGetTitleID());

    OSDynLoad_Error err = OSDynLoad_Acquire(cpy, &TelkinRPLHandle); // load RPL from SD card
    if (err != OS_DYNLOAD_OK) {
        WHBLogPrintf("Couldn't load Telkin.rpl! Err code: %08X\n", err);
        return;
    }

    err = OSDynLoad_FindExport(TelkinRPLHandle, OS_DYNLOAD_EXPORT_FUNC, "init", (void**)&Telkin_init);
    if (err != OS_DYNLOAD_OK) {
        WHBLogPrintf("Couldn't find Telkin's 'init' function! Err code: %08X\n", err);
        return;
    }

    Telkin_init(&OSDynLoad_Acquire, &OSDynLoad_FindExport, &KernWriteWrapper);
    TelkinInitialized = true;
    WHBLogPrintf("Telkin has been initialized!");
    return;
}

void RedirectContentDir() {
    std::string titleIDString = std::format("{:016X}", OSGetTitleID());
    std::string redirContentPath = std::format("/vol/external01/telkin/{}/content/", titleIDString);
    std::string redirDLCPath = std::format("/vol/external01/telkin/{}/aoc/", titleIDString);

    ContentRedirectionStatus ret = ContentRedirection_AddFSLayerEx(&contentLayerHandle, 
        "Telkin FS Redirection - content",
        "/vol/content/",
        redirContentPath.c_str(), 
        FS_LAYER_TYPE_EX_MERGE_DIRECTORY);

    if (ret != CONTENT_REDIRECTION_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to redirect the content dir to %s! Status message: %s", redirContentPath.c_str(), ContentRedirection_GetStatusStr(ret));
    }

    ret = ContentRedirection_AddFSLayer(&aocLayerHandle, 
        "Telkin FS Redirection - DLC",
        redirDLCPath.c_str(), 
        FS_LAYER_TYPE_AOC_MERGE);

    if (ret != CONTENT_REDIRECTION_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to redirect the aoc dir to %s! Status message: %s", redirDLCPath.c_str(), ContentRedirection_GetStatusStr(ret));
    }
}

bool sInitDone = false;

INITIALIZE_PLUGIN() {
    ContentRedirectionStatus error;
    if ((error = ContentRedirection_InitLibrary()) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to init ContentRedirection. Error %s %d", ContentRedirection_GetStatusStr(error), error);
        OSFatal("Failed to init ContentRedirection.");
    }

    sInitDone = true;
}

ON_APPLICATION_START() {
    // Init logging
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }

    RedirectContentDir();
    TelkinBootstrap();
}

ON_APPLICATION_ENDS() {
    if (contentLayerHandle != 0) {
        ContentRedirection_RemoveFSLayer(contentLayerHandle);
        contentLayerHandle = 0;
    }

    if (aocLayerHandle != 0) {
        ContentRedirection_RemoveFSLayer(aocLayerHandle);
        aocLayerHandle = 0;
    }
    TelkinInitialized = 0;
    OSDynLoad_Release(TelkinRPLHandle);
    TelkinRPLHandle = nullptr;
}

DECL_FUNCTION(uint32_t, __OSDynLoad_InternalAcquire, char *name, void **out, uint32_t u1, uint32_t u2, uint32_t u3) {
    uint32_t res = real___OSDynLoad_InternalAcquire(name, out, u1, u2, u3);
    // Make sure the plugin is properly initialized before calling custom code
    if (!sInitDone)
        return res;

    if (res == 0)
        return res;

    // Only for RPL being acquired, so we skip it
    if (strncmp("~|telkin", name, strlen("~|telkin")) == 0)
        return res;

    char cpy[64] = {};
    __os_snprintf(cpy, sizeof(cpy), "~|telkin|%016llX|code|%s", OSGetTitleID(), name);

    res = real___OSDynLoad_InternalAcquire(cpy, out, u1, u2, u3);

    if (res == 0)
        return res;
    else
        return real___OSDynLoad_InternalAcquire(name, out, u1, u2, u3);
}

using LoaderLogFn = void (*)(const char* fmt, ...);
static LoaderLogFn LoaderLog = reinterpret_cast<LoaderLogFn>(0x010028d0);

using __loader_snprintf_t = int (*)(char *buf, size_t n, const char *format, ...);
static __loader_snprintf_t __loader_snprintf = reinterpret_cast<__loader_snprintf_t>(0x01003df8);

DECL_FUNCTION(void*, LiFindRPLByName, char* name) {
    char redirected[64] = {};

    __loader_snprintf(redirected, sizeof(redirected), "~|telkin|%016llX|code|%s", OSGetTitleID(), name);

   // LoaderLog("Trying to find %s\n", redirected);

    auto ret = real_LiFindRPLByName(redirected);
    if (ret) {
       // LoaderLog("Ret RPL: %s\n", ret->moduleNameBuffer);
        return ret;
    }

    return real_LiFindRPLByName(name);
}


WUPS_MUST_REPLACE_PHYSICAL(LiFindRPLByName, 0x32004bc4, 0x01004bc4);
WUPS_MUST_REPLACE_PHYSICAL(__OSDynLoad_InternalAcquire, 0x3201C400 + 0x0cc54, 0x101C400 + 0x0cc54);
