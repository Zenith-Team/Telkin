.origin = codecave

TelkinBootstrap:
    subi r1, r1, 0x8
    mflr r0
    stw r0, 0x4(r1)

    lis r3, Initialized@ha
    addi r3, r3, Initialized@l
    lbz r4, 0x0(r3)
    cmpwi r4, 1
    beqlr

    li r4, 1
    stb r4, 0x0(r3)
    
    # OSDynLoad_Acquire = ExtractAddrFromInstr(&BLOSDynLoad_Acquire);
    lis r3, BLOSDynLoad_Acquire@ha
    addi r3, r3, BLOSDynLoad_Acquire@l
    bl ExtractAddrFromInstr
    lis r4, OSDynLoad_Acquire@ha
    stw r3, OSDynLoad_Acquire@l(r4)

    # OSDynLoad_FindExport = ExtractAddrFromInstr(&BOSDynLoad_FindExport);
    lis r3, BOSDynLoad_FindExport@ha
    addi r3, r3, BOSDynLoad_FindExport@l
    bl ExtractAddrFromInstr
    lis r4, OSDynLoad_FindExport@ha
    stw r3, OSDynLoad_FindExport@l(r4)

    # OSDynLoad_Acquire("telkin", &TelkinRPLHandle);
    lis r3, TelkinRPLName@ha
    addi r3, r3, TelkinRPLName@l
    lis r4, TelkinRPLHandle@ha
    addi r4, r4, TelkinRPLHandle@l
    lis r5, OSDynLoad_Acquire@ha
    lwz r5, OSDynLoad_Acquire@l(r5)
    mtctr r5
    bctrl

    # OSDynLoad_FindExport(TelkinRPLHandle, false, "init", &TelkinInitFuncHandle);
    lis r3, OSDynLoad_FindExport@ha
    lwz r3, OSDynLoad_FindExport@l(r3)
    mtctr r3
    lis r3, TelkinRPLHandle@ha
    lwz r3, TelkinRPLHandle@l(r3)
    li r4, 0
    lis r5, TelkinInitFuncName@ha
    addi r5, r5, TelkinInitFuncName@l
    lis r6, TelkinInitFuncHandle@ha
    addi r6, r6, TelkinInitFuncHandle@l
    bctrl

    # TelkinInitFuncHandle(OSDynLoad_Acquire, OSDynLoad_FindExport);
    lis r3, TelkinInitFuncHandle@ha
    lwz r3, TelkinInitFuncHandle@l(r3)
    mtctr r3
    lis r3, OSDynLoad_Acquire@ha
    lwz r3, OSDynLoad_Acquire@l(r3)
    lis r4, OSDynLoad_FindExport@ha
    lwz r4, OSDynLoad_FindExport@l(r4)
    bctrl

    lwz r0, 0x4(r1)
    mtlr r0
    addi r1, r1, 0x8
    blr

ExtractAddrFromInstr:
    lwz r10, 0x0(r3)
    mr r9, r3
    andi. r8, r10, 0x2
    rlwinm r3, r10, 0, 6, 29
    bnelr cr0
    andis. r10, r10, 0x200
    beq cr0, ExtractAddrFromInstr_Ret
    oris r3, r3, 0xFE00
ExtractAddrFromInstr_Ret:
    add r3, r9, r3
    blr

Initialized:
.byte 0, 0, 0, 0

OSDynLoad_Acquire:
.byte 0, 0, 0, 0

OSDynLoad_FindExport:
.byte 0, 0, 0, 0

TelkinRPLHandle:
.byte 0, 0, 0, 0

TelkinInitFuncHandle:
.byte 0, 0, 0, 0

TelkinRPLName:
.string "telkin"

TelkinInitFuncName:
.string "init"
