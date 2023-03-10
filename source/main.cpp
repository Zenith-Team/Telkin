#include "types.h"
#include "os.h"
#include "fs.h"
#include "loader.h"

#include <cstring>

OsSpecifics osSpecifics;
extern u32 BLOSDynLoad_Acquire;
extern u32 BOSDynLoad_FindExport;

u32 coreinitHandle = 0;

s32 (*OSDynLoad_Acquire)(const char* rpl, u32* handle) = 0;
s32 (*OSDynLoad_FindExport)(u32 handle, s32 isdata, const char* symbol, void* address) = 0;
void* (*OSBlockSet)(void* dst, u8 value, size_t n) = 0;
void (*OSConsoleWrite)(const char* msg, s32 size);
s32 (*__os_snprintf)(char* s, s32 n, const char* format, ...);
u32* MEMAllocFromDefaultHeapExPtr = 0;
u32* MEMAllocFromDefaultHeapPtr = 0;
u32* MEMFreeToDefaultHeapPtr = 0;

s32 (*FSInit)() = 0;
s32 (*FSAddClient)(void* client, s32 errHandling) = 0;
void (*FSInitCmdBlock)(void* cmd) = 0;
s32 (*FSOpenFile)(void* client, void* cmd, const char* path, const char* mode, s32* fd, s32 errHandling) = 0;
s32 (*FSReadFile)(void* client, void* cmd, void* buffer, s32 size, s32 count, s32 fd, s32 flag, s32 errHandling) = 0;
s32 (*FSCloseFile)(void* client, void* cmd, s32 fd, s32 errHandling) = 0;

char logMsg[512];
#define LOG(FMT, ARGS...) do { __os_snprintf(logMsg, sizeof(logMsg), FMT, ## ARGS); OSConsoleWrite(logMsg, sizeof(logMsg)); } while(0)

void initRPL(const char*);

extern "C" void init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    osSpecifics.addr_OSDynLoad_Acquire    = (u32)(BLOSDynLoad_Acquire   & 0x03FFFFFC);
    osSpecifics.addr_OSDynLoad_FindExport = (u32)(BOSDynLoad_FindExport & 0x03FFFFFC);

    if (!(BLOSDynLoad_Acquire & 2))
        osSpecifics.addr_OSDynLoad_Acquire    += (u32)&BLOSDynLoad_Acquire;
    if (!(BOSDynLoad_FindExport & 2))
        osSpecifics.addr_OSDynLoad_FindExport += (u32)&BOSDynLoad_FindExport;

    OSDynLoad_Acquire = (s32 (*)(const char*, u32*))osSpecifics.addr_OSDynLoad_Acquire;
    OSDynLoad_FindExport = (s32 (*)(u32, s32, const char*, void*))osSpecifics.addr_OSDynLoad_FindExport;

    OSDynLoad_Acquire("coreinit.rpl", &coreinitHandle);

    OSDynLoad_FindExport(coreinitHandle, true, "MEMAllocFromDefaultHeapEx", &MEMAllocFromDefaultHeapExPtr);
    OSDynLoad_FindExport(coreinitHandle, true, "MEMAllocFromDefaultHeap", &MEMAllocFromDefaultHeapPtr);
    OSDynLoad_FindExport(coreinitHandle, true, "MEMFreeToDefaultHeap", &MEMFreeToDefaultHeapPtr);

    u32* funcPointer = 0;

    OSDynLoad_FindExport(coreinitHandle, false, "OSBlockSet", &funcPointer); *(u32*)(((u32)&OSBlockSet) + 0) = (u32)funcPointer;
    OSDynLoad_FindExport(coreinitHandle, false, "OSConsoleWrite", &funcPointer); *(u32*)(((u32)&OSConsoleWrite) + 0) = (u32)funcPointer;
    OSDynLoad_FindExport(coreinitHandle, false, "__os_snprintf", &funcPointer); *(u32*)(((u32)&__os_snprintf) + 0) = (u32)funcPointer;

    OSDynLoad_FindExport(coreinitHandle, false, "FSInit", &funcPointer); *(u32*)(((u32)&FSInit) + 0) = (u32)funcPointer;
    OSDynLoad_FindExport(coreinitHandle, false, "FSAddClient", &funcPointer); *(u32*)(((u32)&FSAddClient) + 0) = (u32)funcPointer;
    OSDynLoad_FindExport(coreinitHandle, false, "FSInitCmdBlock", &funcPointer); *(u32*)(((u32)&FSInitCmdBlock) + 0) = (u32)funcPointer;
    OSDynLoad_FindExport(coreinitHandle, false, "FSOpenFile", &funcPointer); *(u32*)(((u32)&FSOpenFile) + 0) = (u32)funcPointer;
    OSDynLoad_FindExport(coreinitHandle, false, "FSReadFile", &funcPointer); *(u32*)(((u32)&FSReadFile) + 0) = (u32)funcPointer;
    OSDynLoad_FindExport(coreinitHandle, false, "FSCloseFile", &funcPointer); *(u32*)(((u32)&FSCloseFile) + 0) = (u32)funcPointer;

    FSInit();

    FSClient* client = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
    if (!client) {
        LOG("Unable to allocate FSClient!");
    }

    FSCmdBlock* cmd = (FSCmdBlock*)MEMAllocFromDefaultHeap(sizeof(FSCmdBlock));
    if (!cmd) {
        LOG("Unable to allocate FSCmdBlock!");
    }

    const u32 BUFFER_SIZE = 1024;

    u8* buffer = (u8*)MEMAllocFromDefaultHeapEx(BUFFER_SIZE, FS_IO_BUFFER_ALIGN);
    if (!buffer) {
        LOG("Unable to allocate buffer!");
    }

    OSBlockSet(buffer, 0, BUFFER_SIZE);

    FSAddClient(client, FS_RET_NO_ERROR);
    FSInitCmdBlock(cmd);

    char path[FS_MAX_ARGPATH_SIZE];
    strncpy(path, "/vol/content/rpl.txt", FS_MAX_ARGPATH_SIZE);

    FSFileHandle handle;
    FSOpenFile(client, cmd, path, "r", &handle, FS_RET_NO_ERROR);
    FSReadFile(client, cmd, buffer, 1, BUFFER_SIZE, handle, 0, FS_RET_NO_ERROR);

    if (*buffer == NULL) {
        LOG("rpl.txt is empty or non-existent, no RPLs to load.");
    } else {
        LOG("RPLs: %s", (char*)buffer);
    }

    char* line = (char*)buffer;
    for (u32 i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';

            initRPL(line);

            line = (char*)(buffer + i + 1);
        }
    }

    FSCloseFile(client, cmd, handle, FS_RET_NO_ERROR);
    MEMFreeToDefaultHeap(client);
    MEMFreeToDefaultHeap(cmd);
    MEMFreeToDefaultHeap(buffer);
}

void initRPL(const char* rplName) {
    //*------------
    //* Step 0: Acquire RPL
    //*------------
    u32 rpl = 0;
    if (OSDynLoad_Acquire(rplName, &rpl)) {
        LOG("Unable to acquire rpl: %s", rplName);
        return;
    }

    //*------------
    //* Step 1: Find start of hooks array
    //*------------
    struct {u32 _[4];}* patches;
    if (OSDynLoad_FindExport(rpl, true, _tLoaderSectionNameData, &patches)) {
        LOG("Unable to find .loaderdata!");
        return;
    }

    LOG(".loaderdata found at: %x", (u32)patches);

    //*------------
    //* Step 2: Apply hooks
    //*------------
    for (u32 i = 0;; i++) {
        switch (patches[i]._[0]) {
            default: LOG("Loader complete, applied %u hooks", i); return;
                
            case tloader::DataMagic::BranchHook: {
                tloader::BranchHook* hook = (tloader::BranchHook*)&patches[i];

                u32 target = 0;
                OSDynLoad_FindExport(rpl, false, hook->target, &target);

                u32 instr = (target - (u32)hook->source) & 0x03FFFFFC;
                    
                switch (hook->type) {
                    case tloader::BranchHook::Type_b:  instr |= 0x48000000; break;
                    case tloader::BranchHook::Type_bl: instr |= 0x48000001; break;
                    default: LOG("INVALID HOOK TYPE FOR HOOK AT: %x", (u32)hook->source); continue;
                }

                *hook->source = instr;

                break;
            }

            case tloader::DataMagic::Patch: {
                tloader::Patch* patch = (tloader::Patch*)&patches[i];
                
                for (u16 i = 0; patch->count != 0; i++, patch->count--) {
                    switch (patch->dataSize) {
                        case 8:  *((u8*) patch->addr) = ((u8*) patch->data)[i]; *((u32*)&patch->addr) += 1; break;
                        case 16: *((u16*)patch->addr) = ((u16*)patch->data)[i]; *((u32*)&patch->addr) += 2; break;
                        case 32: *((u32*)patch->addr) = ((u32*)patch->data)[i]; *((u32*)&patch->addr) += 4; break;
                        case 64: *((u64*)patch->addr) = ((u64*)patch->data)[i]; *((u32*)&patch->addr) += 8; break;
                        default: LOG("INVALID PATCH UNIT SIZE FOR PATCH AT: %x", (u32)patch->addr); break;
                    }
                }
  
                break;
            }

            case tloader::DataMagic::PointerHook: {
                tloader::PointerHook* hook = (tloader::PointerHook*)&patches[i];

                u32 target = 0;
                OSDynLoad_FindExport(rpl, hook->isData, hook->target, &target);

                *hook->source = target;
                
                break;
            }
        }
    }
}
