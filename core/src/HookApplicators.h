#pragma once

#include <telkin/Telkin.h>

namespace tk {
    
    class HookList;
    
    bool applyBranchHook(const tk::BranchHook* hook);
    bool applyPointerHook(const tk::PointerHook* hook);
    bool applyPatchHook(const tk::PatchHook* hook);
    
    bool readBranchHook(u32 rpl, void* hookPtr, tk::HookList& list);
    bool readPointerHook(u32 rpl, void* hookPtr, tk::HookList& list);
    bool readPatchHook(void* hookPtr, tk::HookList& list);
    
} // namespace tk
