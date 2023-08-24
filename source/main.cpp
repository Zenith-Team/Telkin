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
#define LOG(FMT, ARGS...) do { __os_snprintf(logMsg, sizeof(logMsg), "\n" FMT, ## ARGS); OSConsoleWrite(logMsg, sizeof(logMsg)); for (u32 i = 0; i < sizeof(logMsg); i++) {logMsg[i]=0;} } while(0)

void initRPL(const char*);

inline uintptr_t extractAddrFromInstr(const u32* instr) {
    uintptr_t ret = *instr & 0x03FFFFFCU;
    if (!(*instr & 2)) {
        // sign extend offset
        if (ret & 0x02000000U)
        ret |= 0xFE000000U;
        // make relative
        ret += (uintptr_t)instr;
    }
    return ret;
}

extern "C" void __tloader_init() {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    osSpecifics.addr_OSDynLoad_Acquire    = extractAddrFromInstr(&BLOSDynLoad_Acquire);
    osSpecifics.addr_OSDynLoad_FindExport = extractAddrFromInstr(&BOSDynLoad_FindExport);

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

    LOG("Telkin v0.1.0");

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

        goto end;
    } else {
        LOG("RPLs: \n-----\n%s-----", (char*)buffer);
    }

    {
        char* line = (char*)buffer;
        for (u32 i = 0; i < BUFFER_SIZE; i++) {
            if (buffer[i] == '\n') {
                buffer[i] = '\0';

                initRPL(line);

                line = (char*)(buffer + i + 1);
            }
        }
    }

end:
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
        LOG("Unable to acquire RPL: %s", rplName);
        return;
    }

    LOG("Applying RPL: %s", rplName);

    //*------------
    //* Step 1: Find start of hooks array
    //*------------
    typedef tloader::HookList* (*__tloaderGetHookList_t)();
    __tloaderGetHookList_t __tloaderGetHookList = 0;
    int err = OSDynLoad_FindExport(rpl, 0, "__tloaderGetHookList__Fv", &__tloaderGetHookList);
    if (err) {
        LOG("could not find __tloaderGetHookList__Fv, err = 0x%08X\n", err);
        return;
    }
    
    tloader::HookList* hooks = __tloaderGetHookList();
    LOG("Found hooks at 0x%x", hooks);

    //*------------
    //* Step 2: Apply hooks
    //*------------
    struct GenericHook { u32 magic; u32 _[3]; };
    int i = 0;
    for (tloader::HookListNode* hook_it = hooks->head; hook_it; hook_it = hook_it->next) {
        GenericHook* genericHook = (GenericHook*)hook_it->hook;

        switch (genericHook->magic) {
            default: LOG("Applied %u hooks from this RPL", i); return;

            case tloader::DataMagic::BranchHook: {
                tloader::BranchHook* hook = (tloader::BranchHook*)hook_it->hook;

                u32 target = 0;
                OSDynLoad_FindExport(rpl, false, hook->target, &target);

                u32 instr = (target - (u32)hook->source) & 0x03FFFFFC;
                    
                switch (hook->type) {
                    case tloader::BranchHook::Type_b:  instr |= 0x48000000; break;
                    case tloader::BranchHook::Type_bl: instr |= 0x48000001; break;
                    default: LOG("INVALID HOOK TYPE FOR HOOK AT: %x", (u32)hook->source); continue;
                }

                LOG("Writing branch hook: 0x%x to 0x%x", instr, (u32)hook->source);
                *hook->source = instr;

                break;
            }

            case tloader::DataMagic::Patch: {
                tloader::Patch* patch = (tloader::Patch*)hook_it->hook;

                LOG("Applying patch at 0x%x", (u32)patch->data, (u32)patch->addr);

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
                tloader::PointerHook* hook = (tloader::PointerHook*)hook_it->hook;

                u32 target = 0;
                OSDynLoad_FindExport(rpl, hook->isData, hook->target, &target);

                if (target == 0xFFFFFFFF) {
                    LOG("Unable to find symbol: %s", hook->target);
                    continue;
                }

                LOG("Writing pointer hook: %x to %x (target: %s)", target, (u32)hook->source, hook->target);

                *hook->source = target;
                
                break;
            }
        }

        i++;
    }
}
