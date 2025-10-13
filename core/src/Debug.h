#pragma once

#include <dynamic_libs/os_functions.h>

namespace tk {
    constexpr s32 cLogBufferSize = 512;
    extern char logMsg[];
} // namespace tk

inline s32 strlength(const char* str) {
    s32 len = 0;
    while (*str++)
        len++;
    return len;
}

#define LOG(FMT, ...) do {                                                                              \
    __os_snprintf(tk::logMsg, tk::cLogBufferSize, "" FMT "\n", ## __VA_ARGS__);                         \
    OSConsoleWrite(tk::logMsg, strlength(tk::logMsg)); OSBlockSet(tk::logMsg, 0, tk::cLogBufferSize);   \
} while (0)

