#include "HookApplicators.h"

#include "Debug.h"
#include "Lists.h"

bool tk::applyBranchHook(const tk::BranchHook* hook) {
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 instr = (reinterpret_cast<u32>(hook->target) - addr) & 0x03FFFFFC; // TODO: Validate range

    switch (hook->type) {
        default: {
            LOG("Invalid hook type for hook at: 0x%08X", addr);
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

    LOG("Writing branch hook: 0x%08X to 0x%08X", instr, addr);
    *hook->source = instr;

    DCFlushRange(hook->source, sizeof(instr));
    ICInvalidateRange(hook->source, sizeof(instr));
    asm volatile("isync" : : : "memory");

    return true;
}

bool tk::applyPointerHook(const tk::PointerHook* hook) {
    const u32 addr = reinterpret_cast<u32>(hook->source);

    LOG("Writing pointer hook: 0x%08X to 0x%08X", addr, hook->target);

    *hook->source = reinterpret_cast<u32>(hook->target);

    DCFlushRange(hook->source, sizeof(void*));

    return true;
}

bool tk::applyPatchHook(const tk::PatchHook* patch) {
    const u32 addr = reinterpret_cast<u32>(patch->addr);
    const u32 totalSize = patch->count * (patch->dataSize / 8);

    LOG("Applying patch at 0x%08X", addr);

    switch (patch->dataSize) {
        default: {
            LOG("Invalid patch unit size %u at addr 0x%08X", patch->dataSize, addr);
            return false;
        }

        case 8: {
            const u8* src = reinterpret_cast<const u8*>(patch->data);
            for (u16 i = 0; i < patch->count; i++)
                reinterpret_cast<u8*>(addr)[i] = src[i];
            break;
        }

        case 16: {
            const u16* src = reinterpret_cast<const u16*>(patch->data);
            for (u16 i = 0; i < patch->count; i++)
                reinterpret_cast<u16*>(addr)[i] = src[i];
            break;
        }

        case 32: {
            const u32* src = reinterpret_cast<const u32*>(patch->data);
            for (u16 i = 0; i < patch->count; i++)
                reinterpret_cast<u32*>(addr)[i] = src[i];
            break;
        }
    }

    DCFlushRange(patch->addr, totalSize);
    ICInvalidateRange(patch->addr, totalSize);
    asm volatile("isync" : : : "memory");

    return true;
}

bool tk::readBranchHook(u32 rpl, void* hookPtr, HookList& list) {
    tk::BranchHook* hook = reinterpret_cast<tk::BranchHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 target = 0;
    s32 err = OSDynLoad_FindExport(rpl, false, hook->target, &target);
    if (err != 0 || target == 0 || target == 0xFFFFFFFF) {
        LOG("Could not find branch hook target: %s for patch at: 0x%08X", hook->target, addr);
        return false;
    }

    hook->target = reinterpret_cast<const char*>(target); //* We are resolving this string early while we still have access to the RPL and reusing the pointer field for the final address
    
    list.add(hook, addr, addr + sizeof(u32));

    return true;
}

bool tk::readPointerHook(u32 rpl, void* hookPtr, HookList& list) {
    tk::PointerHook* hook = reinterpret_cast<tk::PointerHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(hook->source);

    u32 target = 0;
    s32 err = OSDynLoad_FindExport(rpl, hook->isData, hook->target, &target);
    if (err != 0 || target == 0 || target == 0xFFFFFFFF) {
        LOG("Could not find pointer hook target: %s for patch at: 0x%08X", hook->target, addr);
        return false;
    }

    hook->target = reinterpret_cast<const char*>(target); //* We are resolving this string early while we still have access to the RPL and reusing the pointer field for the final address

    list.add(hook, addr, addr + sizeof(void*));

    return true;
}

bool tk::readPatchHook(void* hookPtr, HookList& list) {
    const tk::PatchHook* patch = reinterpret_cast<tk::PatchHook*>(hookPtr);
    const u32 addr = reinterpret_cast<u32>(patch->addr);
    const u32 totalSize = patch->count * (patch->dataSize / 8);

    switch (patch->dataSize) {
        default: {
            LOG("Invalid patch unit size %u at addr 0x%08X", patch->dataSize, addr);
            return false;
        }

        case 8:
        case 16:
        case 32:
            break;
    }
    
    list.add(patch, addr, addr + totalSize);

    return true;
}
