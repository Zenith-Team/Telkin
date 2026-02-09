; Telkin Loader
; Copyright (c) 2023-2026 Zenith Team - MPL-2.0-no-copyleft-exception
[TelkinLauncher_Cemu]
moduleMatches=rpx

.origin = codecave

.callback entry TelkinBootstrap
TelkinBootstrap:
    # Allocate stack frame
    subi r1, r1, 0x10

    # Store LR
    mflr r0
    stw r0, 0x4(r1)

    # if (Initialized) return
    lis r3, Initialized@ha
    addi r3, r3, Initialized@l
    lbz r4, 0x0(r3)
    cmpwi r4, 1
    beqlr

    # Initialized = true
    li r4, 1
    stb r4, 0x0(r3)

    # OSDynLoad_Acquire("telkin", &TelkinRPLHandle);
    lis r3, TelkinRPLName@ha
    addi r3, r3, TelkinRPLName@l
    lis r4, TelkinRPLHandle@ha
    addi r4, r4, TelkinRPLHandle@l
    bl import.coreinit.OSDynLoad_Acquire

    # OSDynLoad_FindExport(TelkinRPLHandle, false, "init", &TelkinInitFuncHandle);
    lis r3, TelkinRPLHandle@ha
    lwz r3, TelkinRPLHandle@l(r3)
    li r4, 0
    lis r5, TelkinInitFuncName@ha
    addi r5, r5, TelkinInitFuncName@l
    lis r6, TelkinInitFuncHandle@ha
    addi r6, r6, TelkinInitFuncHandle@l
    bl import.coreinit.OSDynLoad_FindExport

    # TelkinInitFuncHandle(OSDynLoad_Acquire, OSDynLoad_FindExport, nullptr);
    lis r3, TelkinInitFuncHandle@ha
    lwz r3, TelkinInitFuncHandle@l(r3)
    mtctr r3
    lis r3, import.coreinit.OSDynLoad_Acquire@ha
    addi r3, r3, import.coreinit.OSDynLoad_Acquire@l
    lis r4, import.coreinit.OSDynLoad_FindExport@ha
    addi r4, r4, import.coreinit.OSDynLoad_FindExport@l
    li r5, 0x0
    bctrl

    # Restore LR
    lwz r0, 0x4(r1)
    mtlr r0

    # Deallocate stack frame
    addi r1, r1, 0x10

    # Return (GG!)
    blr

Initialized:
.long 0

TelkinRPLHandle:
.long 0

TelkinInitFuncHandle:
.long 0

TelkinRPLName:
.string "Telkin"

TelkinInitFuncName:
.string "init"
