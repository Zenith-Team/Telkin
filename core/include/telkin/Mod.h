#pragma once

#include <dynamic_libs/os_types.h>
#include <span>

namespace tk {
    enum class ModuleType : u32 {
        Null,
        Special,
        CoreMod,
        CoreAPI,
        Standard
    };
    
    using startfunc_t = void (*)(u32, u32);
    using getTitleID_t = u64 (*)();
    using getModID_t = const char* (*)();
    using getModuleType_t = ModuleType (*)();
    using getDependencyManifest_t = const u8* (*)();
    
    struct ModInfo {
        const char* id;
        const char* version;
    };
    
    const std::span<ModInfo> getMods();
    bool isModLoaded(const char* id);
}
