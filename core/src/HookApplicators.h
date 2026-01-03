#pragma once

#include <telkin/Telkin.h>

#include <vector>

namespace tk {
    
    class GenericHook;
    
    struct HookEntry {
        const tk::GenericHook* hook;
        u32 startAddr;
        u32 endAddr;
    };
    
    bool applyBranchHook(const tk::BranchHook* hook);
    bool applyPointerHook(const tk::PointerHook* hook);
    bool applyPatchHook(const tk::PatchHook* hook);
    
    bool readBranchHook(u32 rpl, void* hookPtr, std::vector<HookEntry>& list);
    bool readPointerHook(u32 rpl, void* hookPtr, std::vector<HookEntry>& list);
    bool readPatchHook(void* hookPtr, std::vector<HookEntry>& list);
    
} // namespace tk
