#pragma once

#include <cafe.h>

#include <source_location>
#include <utility>

namespace tk {
    
    namespace internal {
        constexpr const char* basename(const char* path) {
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

    namespace LogColor {
        constexpr const char* Reset        = "\33[0m";
        constexpr const char* Bold         = "\33[1m";
        constexpr const char* NoBold       = "\33[22m";
        constexpr const char* Underline    = "\33[4m";
        constexpr const char* NoUnderline  = "\33[24m";
        constexpr const char* Invert       = "\33[7m";
        constexpr const char* NoInvert     = "\33[27m";
    
        constexpr const char* Black        = "\33[30m";
        constexpr const char* Red          = "\33[31m";
        constexpr const char* Green        = "\33[32m";
        constexpr const char* Yellow       = "\33[33m";
        constexpr const char* Blue         = "\33[34m";
        constexpr const char* Magenta      = "\33[35m";
        constexpr const char* Cyan         = "\33[36m";
        constexpr const char* LightGray    = "\33[37m";
        constexpr const char* Gray         = "\33[90m";
        constexpr const char* LightRed     = "\33[91m";
        constexpr const char* LightGreen   = "\33[92m";
        constexpr const char* LightYellow  = "\33[93m";
        constexpr const char* LightBlue    = "\33[94m";
        constexpr const char* LightMagenta = "\33[95m";
        constexpr const char* LightCyan    = "\33[96m";
        constexpr const char* White        = "\33[97m";
        constexpr const char* Default      = "\33[39m";
        //constexpr const char* Extended     = "\33[38m";

        namespace BG {
            constexpr const char* Black        = "\33[40m";
            constexpr const char* Red          = "\33[41m";
            constexpr const char* Green        = "\33[42m";
            constexpr const char* Yellow       = "\33[43m";
            constexpr const char* Blue         = "\33[44m";
            constexpr const char* Magenta      = "\33[45m";
            constexpr const char* Cyan         = "\33[46m";
            constexpr const char* LightGray    = "\33[47m";
            constexpr const char* Gray         = "\33[100m";
            constexpr const char* LightRed     = "\33[101m";
            constexpr const char* LightGreen   = "\33[102m";
            constexpr const char* LightYellow  = "\33[103m";
            constexpr const char* LightBlue    = "\33[104m";
            constexpr const char* LightMagenta = "\33[105m";
            constexpr const char* LightCyan    = "\33[106m";
            constexpr const char* White        = "\33[107m";
            constexpr const char* Default      = "\33[49m";
            //constexpr const char* Extended     = "\33[48m";
        }
    }

    template <typename... Args>
    void print(internal::LogFormat format, Args&&... args) {
        const auto& fmt = format.fmt;
        
        OSReport("[%s:%d] ", format.file, format.line);
        OSReport(fmt, std::forward<Args>(args)...);
    }
    
    template <typename... Args>
    void println(internal::LogFormat format, Args&&... args) {
        tk::print(format, std::forward<Args>(args)...);
        OSReport("%s\n", LogColor::Reset);
    }
    
    template <typename... Args>
    void warn(internal::LogFormat format, Args&&... args) {
        OSReport("%s", LogColor::Yellow);
        println(format, std::forward<Args>(args)...);
    }
    
    template <typename... Args>
    void fatal(internal::LogFormat format, Args&&... args) {
        const auto& fmt = format.fmt;
        
        OSReport("%s[%s:%d] ERROR: ", LogColor::Magenta, format.file, format.line);
        OSReport(fmt, std::forward<Args>(args)...);
        OSReport("%s\n", LogColor::Reset); // we don't expect anything else afterwards
        
        static char buf[512];
        
        __os_snprintf(buf, sizeof(buf), fmt, std::forward<Args>(args)...);
        OSFatal(buf);
    }
}
