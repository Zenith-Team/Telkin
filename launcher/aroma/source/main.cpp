#include <string.h>
#include <format>

#include <wups.h>
#include <whb/log.h>
#include <whb/log_module.h>
#include <whb/log_cafe.h>
#include <whb/log_udp.h>
#include <function_patcher/function_patching.h>
#include <content_redirection/redirection.h>

#include <coreinit/cache.h>
#include <coreinit/title.h>

#include <patcher/rplinfo.h>

WUPS_PLUGIN_NAME("Telkin RPL Loader");
WUPS_PLUGIN_DESCRIPTION("<to be written>");
WUPS_PLUGIN_VERSION("prerelease");
WUPS_PLUGIN_AUTHOR("techmuse, Luminyx");
WUPS_PLUGIN_LICENSE("MPL-2.0");

WUPS_USE_WUT_DEVOPTAB();

#define NSMBU_US_TID 0x0005000010101D00llu // TEMP
static int Initialized = 0;
static uint32_t TelkinRPLHandle;
static CRLayerHandle contentLayerHandle;

static PatchedFunctionHandle patchHandle;

void TelkinBootstrap() {
    WHBLogPrintf("In the Telkin bootstrap!\n");
    if (Initialized)
        return;

    typedef void (*TelkinInitFuncHandle_t)(void *acquireAddr, void *exportAddr);
    TelkinInitFuncHandle_t TelkinInitFuncHandle;
    // The RPL doesn't load properly on console yet so this is out for now

    /* OSDynLoad_Error err = OSDynLoad_Acquire("~/Telkin.rpl", (OSDynLoad_Module*)&TelkinRPLHandle); // load RPL from SD card
    if (err != OS_DYNLOAD_OK) {
        WHBLogPrintf("Couldn't load Telkin.rpl! Err: %08X\n", err);
    }

    OSDynLoad_FindExport((OSDynLoad_Module*)&TelkinRPLHandle, OS_DYNLOAD_EXPORT_FUNC, "init", (void**)TelkinInitFuncHandle);

    TelkinInitFuncHandle((void*)&OSDynLoad_Acquire, (void*)&OSDynLoad_FindExport);
    */
    return;
}

DECL_FUNCTION(void, call_ctors) {
    WHBLogPrintf("In call ctors hook\n");
    real_call_ctors();
    WHBLogPrintf("Post real call ctors\n");
    TelkinBootstrap();
    return;
}

void RedirectContentDir() {
    std::string titleIDString = std::format("{:016X}", OSGetTitleID());
    std::string layerName = "Telkin FS Redirection";
    // TODO: Add DLC redirection support (and possibly saves as well?)
    std::string redirPath = std::format("/vol/external01/telkin/{}/content/", titleIDString);

    auto ret = ContentRedirection_AddFSLayerEx(&contentLayerHandle, 
        layerName.c_str(),
        "/vol/content/",
        redirPath.c_str(), 
        FS_LAYER_TYPE_EX_MERGE_DIRECTORY);

    if (ret != CONTENT_REDIRECTION_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to redirect the content dir to %s!\n", redirPath.c_str());
    }
}

INITIALIZE_PLUGIN() {
    WHBLogPrintf("Patch functions");
    if (FunctionPatcher_InitLibrary() != FUNCTION_PATCHER_RESULT_SUCCESS) {
        OSFatal("Telkin RPL Loader: FunctionPatcher_InitLibrary failed");
    }

    ContentRedirectionStatus error;
    if ((error = ContentRedirection_InitLibrary()) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to init ContentRedirection. Error %s %d", ContentRedirection_GetStatusStr(error), error);
        OSFatal("Failed to init ContentRedirection.");
    }

}

DEINITIALIZE_PLUGIN() {
    FunctionPatcher_RemoveFunctionPatch(patchHandle);
}

ON_APPLICATION_START() {
    if (OSGetTitleID() != NSMBU_US_TID) {
        return;
    }
    
    // Init logging
    if (!WHBLogModuleInit()) {
        WHBLogCafeInit();
        WHBLogUdpInit();
    }

    WHBLogPrintf("Telkin: applying patches...");
    /* 
    Early attempts (might get removed?)

    // Patch the dynload functions so GetRPLInfo works
    if (!PatchDynLoadFunctions()) {
        WHBLogPrintf("Telkin: Failed to patch dynload functions");
        return;
    }

    // Get the RPLInfo
    auto rpl_info = TryGetRPLInfo();
    if (!rpl_info) {
        WHBLogPrintf("Telkin: Failed to get RPL info");
        return;
    }

    // Find the rpx
    rplinfo rpls = *rpl_info;
    auto red_pro2_rpx = FindRPL(rpls, "red-pro2.rpx");
    if (!red_pro2_rpx) {
        WHBLogPrintf("Telkin: Failed to find red-pro2.rpx");
        return;
    }

    OSDynLoad_NotifyData rpx_data = *red_pro2_rpx;
    uint32_t inject_addr = rpx_data.textAddr + 0xaf9fb0; 
    */

    uint64_t titleIds[]              = {NSMBU_US_TID};
    function_replacement_data_t repl = REPLACE_FUNCTION_OF_EXECUTABLE_BY_ADDRESS_WITH_VERSION(
            call_ctors,
            titleIds, sizeof(titleIds) / sizeof(titleIds[0]),
            "red-pro2.rpx",
            0xaf9fd0, // Address of 'call_ctors'
            64, 64);

    if (FunctionPatcher_AddFunctionPatch(&repl, &patchHandle, nullptr) != FUNCTION_PATCHER_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to add \"call_ctors\" patch\n");
    }

    RedirectContentDir();

}

ON_APPLICATION_ENDS() {
    if (contentLayerHandle != 0) {
        ContentRedirection_RemoveFSLayer(contentLayerHandle);
        contentLayerHandle = 0;
    }

}
