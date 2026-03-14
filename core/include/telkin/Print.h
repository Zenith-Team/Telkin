#pragma once

#include <cafe.h>

#include <source_location>
#include <utility>

namespace tk {
    
    namespace internal {
        consteval const char* basename(const char* path) {
            const char* last = path;
        
            for (const char* p = path; *p; ++p) {
                if (*p == '/' || *p == '\\') {
                    last = p + 1;
                }
            }
        
            return last;
        }
        
        struct LogFormat {
            const char* fmt;
            const char* file;
            u32 line;
            
            consteval LogFormat(const char* str, std::source_location loc = std::source_location::current())
                : fmt(str)
                , file(basename(loc.file_name()))
                , line(loc.line())
            { }
        };
    }
    
    template <typename... Args>
    void print(internal::LogFormat format, Args&&... args) {
        const auto& fmt = format.fmt;
        
        OSReport("[%s:%d] ", format.file, format.line);
        OSReport(fmt, std::forward<Args>(args)...);
    }
}
