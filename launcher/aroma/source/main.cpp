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

WUPS_PLUGIN_NAME("Telkin RPL Loader");
WUPS_PLUGIN_DESCRIPTION("A dynamic Wii U mod loader.");
WUPS_PLUGIN_VERSION("prerelease");
WUPS_PLUGIN_AUTHOR("techmuse, Luminyx");
WUPS_PLUGIN_LICENSE("MPL-2.0");

WUPS_USE_WUT_DEVOPTAB();

static bool TelkinInitialized = 0;
OSDynLoad_Module TelkinRPLHandle = nullptr;
static CRLayerHandle contentLayerHandle;

void TelkinBootstrap() {
    WHBLogPrintf("In the Telkin bootstrap!\n");
    if (TelkinInitialized)
        return;

    typedef void (*Telkin_init_t)(void *acquireAddr, void *exportAddr);
    Telkin_init_t Telkin_init;

    OSDynLoad_Error err = OSDynLoad_Acquire("Telkin.rpl", &TelkinRPLHandle); // load RPL from SD card
    if (err != OS_DYNLOAD_OK) {
        WHBLogPrintf("Couldn't load Telkin.rpl! Err code: %08X\n", err);
        return;
    }

    err = OSDynLoad_FindExport(&TelkinRPLHandle, OS_DYNLOAD_EXPORT_FUNC, "init", (void**)&Telkin_init);
    if (err != OS_DYNLOAD_OK) {
        WHBLogPrintf("Couldn't find Telkin's 'init' function! Err code: %08X\n", err);
        return;
    }

    Telkin_init((void*)&OSDynLoad_Acquire, (void**)&OSDynLoad_FindExport);
    TelkinInitialized = true;
    WHBLogPrintf("Telkin has been initialized!");
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
    ContentRedirectionStatus error;
    if ((error = ContentRedirection_InitLibrary()) != CONTENT_REDIRECTION_RESULT_SUCCESS) {
        WHBLogPrintf("Failed to init ContentRedirection. Error %s %d", ContentRedirection_GetStatusStr(error), error);
        OSFatal("Failed to init ContentRedirection.");
    }
}

DEINITIALIZE_PLUGIN() {
  //...
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

    OSDynLoad_Release(TelkinRPLHandle);
    TelkinRPLHandle = nullptr;
}
