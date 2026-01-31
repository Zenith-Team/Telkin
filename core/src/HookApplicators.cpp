#include "HookApplicators.h"
#include "PrivateInterface.h"

#include <dynamic_libs/os_functions.h>

extern "C" void MAGIC_CALLBACK() {
    // defined externally for no-inline
}

bool tk::applyBranchHook(const tk::BranchHook* hook) {
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 instr = (reinterpret_cast<u32>(hook->target) - addr) & 0x03FFFFFC;
    const s32 offset = reinterpret_cast<u32>(hook->target) - addr;
    if (offset > 0x01FFFFFC || offset < -0x02000000) {
        OSReport("Hook target out of range (diff: %d)\n", offset);
        return false; 
    }

    switch (hook->type) {
        default: {
            OSReport("Invalid hook type for hook at: 0x%08X\n", addr);
            return false;
        }

        case tk::BranchType::b: {
            instr |= 0x48000000;
            break;
        }

        case tk::BranchType::bl: {
            instr |= 0x48000001;
            break;
        }
    }

    OSReport("Writing branch hook: 0x%08X to 0x%08X\n", instr, addr);
    
    tk::sPrivilegedWrite(hook->source, &instr, sizeof(u32));
    
    DCFlushRange(hook->source, sizeof(instr));
    ICInvalidateRange(hook->source, sizeof(instr));
    asm volatile("isync" : : : "memory");

    return true;
}

bool tk::applyPointerHook(const tk::PointerHook* hook) {
    const u32 addr = reinterpret_cast<u32>(hook->source);

    OSReport("Writing pointer hook: 0x%08X to 0x%08X\n", addr, hook->target);

    tk::sPrivilegedWrite(hook->source, (void*)&hook->target, sizeof(u32));

    DCFlushRange(hook->source, sizeof(void*));

    return true;
}

bool tk::applyPatchHook(const tk::PatchHook* patch) {
    const u32 addr = reinterpret_cast<u32>(patch->addr);
    const u32 totalSize = patch->count * (patch->dataSize / 8);

    OSReport("Applying patch at 0x%08X\n", addr);

    tk::sPrivilegedWrite(patch->addr, (void*)patch->data, totalSize);

    DCFlushRange(patch->addr, totalSize);
    ICInvalidateRange(patch->addr, totalSize);
    asm volatile("isync" : : : "memory");

    return true;
}

bool tk::readBranchHook(u32 rpl, void* hookPtr, std::vector<HookEntry>& list, std::vector<HookEntry>& listFull) {
    tk::BranchHook* hook = reinterpret_cast<tk::BranchHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 target = 0;
    s32 err = OSDynLoad_FindExport(rpl, false, hook->target, &target);
    if (err != 0 || target == 0 || target == 0xFFFFFFFF) {
        OSReport("Could not find branch hook target: %s for patch at: 0x%08X\n", hook->target, addr);
        return false;
    }

    hook->target = reinterpret_cast<const char*>(target); //* We are resolving this string early while we still have access to the RPL and reusing the pointer field for the final address
    
    list.emplace_back((GenericHook*)hook, addr, addr + sizeof(u32));
    listFull.emplace_back((GenericHook*)hook, addr, addr + sizeof(u32));

    return true;
}

bool tk::readPointerHook(u32 rpl, void* hookPtr, std::vector<HookEntry>& list, std::vector<HookEntry>& listFull) {
    tk::PointerHook* hook = reinterpret_cast<tk::PointerHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 target = 0;
    s32 err = OSDynLoad_FindExport(rpl, hook->isData, hook->target, &target);
    if (err != 0 || target == 0 || target == 0xFFFFFFFF) {
        OSReport("Could not find pointer hook target: %s for patch at: 0x%08X\n", hook->target, addr);
        return false;
    }

    hook->target = reinterpret_cast<const char*>(target); //* We are resolving this string early while we still have access to the RPL and reusing the pointer field for the final address

    list.emplace_back((GenericHook*)hook, addr, addr + sizeof(void*));
    listFull.emplace_back((GenericHook*)hook, addr, addr + sizeof(void*));

    return true;
}

bool tk::readPatchHook(void* hookPtr, std::vector<HookEntry>& list, std::vector<HookEntry>& listFull) {
    const tk::PatchHook* patch = reinterpret_cast<tk::PatchHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(patch->addr);
    const u32 totalSize = patch->count * (patch->dataSize / 8);

    switch (patch->dataSize) {
        default: {
            OSReport("Invalid patch unit size %u at addr 0x%08X\n", patch->dataSize, addr);
            return false;
        }

        case 8:
        case 16:
        case 32:
            break;
    }
    
    list.emplace_back((GenericHook*)patch, addr, addr + totalSize);
    listFull.emplace_back((GenericHook*)patch, addr, addr + totalSize);

    return true;
}
