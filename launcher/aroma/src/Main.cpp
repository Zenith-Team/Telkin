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
#include <coreinit/title.h>
#include <coreinit/memorymap.h>

WUPS_PLUGIN_NAME("Telkin RPL Loader");
WUPS_PLUGIN_DESCRIPTION("A dynamic Wii U mod loader.");
WUPS_PLUGIN_VERSION("prerelease");
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

    OSDynLoad_Error err = OSDynLoad_Acquire("Telkin.rpl", &TelkinRPLHandle); // load RPL from SD card
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

INITIALIZE_PLUGIN() {
    ContentRedirectionStatus error;
    if ((error = ContentRedirection_InitLibrary()) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to init ContentRedirection. Error %s %d", ContentRedirection_GetStatusStr(error), error);
        OSFatal("Failed to init ContentRedirection.");
    }
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

    OSDynLoad_Release(TelkinRPLHandle);
    TelkinRPLHandle = nullptr;
}
