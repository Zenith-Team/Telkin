#pragma once

#include <telkin/Preprocessor.h>

// thx to mkwcat/nsmbw-project for some of these

#define tAssembly(...) __attribute__((naked)) __attribute__((__noinline__)) { __asm__(PP_STR_VAL(__VA_ARGS__)); }

#define tRegSave __attribute__((preserve_all)) // red hills compiler magic for ppc

#ifdef TELKIN_REGISTERS
#define r0 %r0
#define r1 %r1
#define sp %r1
#define r2 %r2
#define r3 %r3
#define r4 %r4
#define r5 %r5
#define r6 %r6
#define r7 %r7
#define r8 %r8
#define r9 %r9
#define r10 %r10
#define r11 %r11
#define r12 %r12
#define r13 %r13
#define r14 %r14
#define r15 %r15
#define r16 %r16
#define r17 %r17
#define r18 %r18
#define r19 %r19
#define r20 %r20
#define r21 %r21
#define r22 %r22
#define r23 %r23
#define r24 %r24
#define r25 %r25
#define r26 %r26
#define r27 %r27
#define r28 %r28
#define r29 %r29
#define r30 %r30
#define r31 %r31

#define f0 %f0
#define f1 %f1
#define f2 %f2
#define f3 %f3
#define f4 %f4
#define f5 %f5
#define f6 %f6
#define f7 %f7
#define f8 %f8
#define f9 %f9
#define f10 %f10
#define f11 %f11
#define f12 %f12
#define f13 %f13
#define f14 %f14
#define f15 %f15
#define f16 %f16
#define f17 %f17
#define f18 %f18
#define f19 %f19
#define f20 %f20
#define f21 %f21
#define f22 %f22
#define f23 %f23
#define f24 %f24
#define f25 %f25
#define f26 %f26
#define f27 %f27
#define f28 %f28
#define f29 %f29
#define f30 %f30
#define f31 %f31

// It's recommended to use the tRegSave attribute directly in C++ instead of the below macros
#define tSaveVolatileRegisters  \
    stwu  r1, -0x3C(r1);        \
    stw   r0,  0x08(r1);        \
    stw   r3,  0x0C(r1);        \
    stw   r4,  0x10(r1);        \
    stw   r5,  0x14(r1);        \
    stw   r6,  0x18(r1);        \
    stw   r7,  0x1C(r1);        \
    stw   r8,  0x20(r1);        \
    stw   r9,  0x24(r1);        \
    stw   r10, 0x28(r1);        \
    stw   r11, 0x2C(r1);        \
    stw   r12, 0x30(r1);        \
    mfcr  r0;                    \
    stw   r0,  0x34(r1);        \
    mfctr r0;                    \
    stw   r0,  0x38(r1);        \
    mflr  r0;                    \
    stw   r0,  0x40(r1)

#define tRestoreVolatileRegisters \
    lwz   r0,  0x38(r1);        \
    mtctr r0;                    \
    lwz   r0,  0x34(r1);        \
    mtcr  r0;                    \
    lwz   r0,  0x40(r1);        \
    mtlr  r0;                    \
    lwz   r0,  0x08(r1);        \
    lwz   r3,  0x0C(r1);        \
    lwz   r4,  0x10(r1);        \
    lwz   r5,  0x14(r1);        \
    lwz   r6,  0x18(r1);        \
    lwz   r7,  0x1C(r1);        \
    lwz   r8,  0x20(r1);        \
    lwz   r9,  0x24(r1);        \
    lwz   r10, 0x28(r1);        \
    lwz   r11, 0x2C(r1);        \
    lwz   r12, 0x30(r1);        \
    addi  r1,  r1, 0x3C

#define tSaveLR \
    stwu r1, -0x10(r1) \
    mflr r2 \
    stw r2, 0x14(r1)

#define tRestoreLR \
    lwz r2, 0x14(r1) \
    mtlr r2 \
    addi r1, r1, 0x10

#endif // TELKIN_REGISTERS
