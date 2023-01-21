#pragma once

#include "types.h"

extern "C" {

#define FS_IO_BUFFER_ALIGN 64
#define FS_RET_NO_ERROR 0
#define FS_MAX_LOCALPATH_SIZE           511
#define FS_MAX_MOUNTPATH_SIZE           128
#define FS_MAX_FULLPATH_SIZE            (FS_MAX_LOCALPATH_SIZE + FS_MAX_MOUNTPATH_SIZE)
#define FS_MAX_ARGPATH_SIZE             FS_MAX_FULLPATH_SIZE

typedef s32 FSFileHandle;

struct FSClient {
    u8 buffer[0x1700];
};

struct FSCmdBlock {
    u8 buffer[0xA80];
};

extern s32 (*FSInit)();
extern s32 (*FSAddClient)(void* client, s32 errHandling);
extern void (*FSInitCmdBlock)(void* cmd);
extern s32 (*FSOpenFile)(void* client, void* cmd, const char* path, const char* mode, s32* fd, s32 errHandling);
extern s32 (*FSReadFile)(void* client, void* cmd, void* buffer, s32 size, s32 count, s32 fd, s32 flag, s32 errHandling);
extern s32 (*FSCloseFile)(void* client, void* cmd, s32 fd, s32 errHandling);

}
