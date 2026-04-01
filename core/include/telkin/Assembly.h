#pragma once

#include <telkin/Preprocessor.h>
#include <cafe.h>

// thx to mkwcat/nsmbw-project for some of these

#ifdef TELKIN_NO_REGISTERS
    #define TELKIN_REGISTERS_WARNING()
#else
    #define TELKIN_REGISTERS_WARNING() _Pragma("message \"WARNING: TELKIN_REGISTERS was not defined. It is recommended to globally define TELKIN_NO_REGISTERS if this was intentional\"")
#endif

#ifdef TELKIN_REGISTERS
    #define tAssembly(...) __attribute__((naked)) __attribute__((__noinline__)) { __asm__ volatile (PP_STR_VAL(__VA_ARGS__)); }
#else
    #define tAssembly(...) \
        TELKIN_REGISTERS_WARNING() \
        __attribute__((naked)) __attribute__((__noinline__)) { __asm__ volatile (PP_STR_VAL(__VA_ARGS__)); }
#endif

#ifdef __clangd__
    #define tRegSave __attribute__(())
#else
    #define tRegSave __attribute__((preserve_all)) // red hills compiler magic for ppc
#endif

namespace tk::ppc {

    enum class GPR : u8 {
        r0,  r1,  r2,  r3,  r4,
        r5,  r6,  r7,  r8,  r9,
        r10, r11, r12, r13, r14,
        r15, r16, r17, r18, r19,
        r20, r21, r22, r23, r24,
        r25, r26, r27, r28, r29,
        r30, r31,
        
        sp = r1
    };
    
    enum class FPR : u8 {
        f0,  f1,  f2,  f3,  f4,
        f5,  f6,  f7,  f8,  f9,
        f10, f11, f12, f13, f14,
        f15, f16, f17, f18, f19,
        f20, f21, f22, f23, f24,
        f25, f26, f27, f28, f29,
        f30, f31
    };
    
    enum class CR : u8 {
        cr0, cr1, cr2, cr3, cr4, cr5, cr6, cr7
    };
    
    using R = GPR;
    
    namespace internal {
        // I-form: [ op:6 | LI:24 | AA:1 | LK:1 ]
        consteval u32 I_form(u8 op, s32 li, bool aa, bool lk) {
            return (u32(op) << 26)
                | (u32(li & 0xFF'FF'FF) << 2)
                | (u32(aa) << 1)
                | u32(lk);
        }
    
        // B-form: [ op:6 | BO:5 | BI:5 | BD:14 | AA:1 | LK:1 ]
        consteval u32 B_form(u8 op, u8 bo, u8 bi, s16 bd, bool aa, bool lk) {
            return (u32(op) << 26)
                | (u32(bo & 0x1F) << 21)
                | (u32(bi & 0x1F) << 16)
                | (u32(bd & 0x3FFF) << 2)
                | (u32(aa) << 1)
                | u32(lk);
        }
    
        // D-form: [ op:6 | RT/RS:5 | RA:5 | D:16 ]
        consteval u32 D_form(u8 op, u8 rsd, u8 ra, s16 d) {
            return (u32(op) << 26)
                | (u32(rsd & 0x1F) << 21)
                | (u32(ra & 0x1F) << 16)
                | u16(d);
        }
    
        // X-form: [ op:6 | RT/RS:5 | RA:5 | RB:5 | XO:10 | Rc:1 ]
        consteval u32 X_form(u8 op, u8 rsd, u8 ra, u8 rb, u16 xo, bool rc) {
            return (u32(op) << 26)
                | (u32(rsd & 0x1F) << 21)
                | (u32(ra & 0x1F) << 16)
                | (u32(rb & 0x1F) << 11)
                | (u32(xo & 0x3FF) << 1)
                | u32(rc);
        }
    
        // XO-form: [ op:6 | RT:5 | RA:5 | RB:5 | OE:1 | XO:9 | Rc:1 ]
        consteval u32 XO_form(u8 op, u8 rt, u8 ra, u8 rb, bool oe, u16 xo, bool rc) {
            return (u32(op) << 26)
                | (u32(rt & 0x1F) << 21)
                | (u32(ra & 0x1F) << 16)
                | (u32(rb & 0x1F) << 11)
                | (u32(oe) << 10)
                | (u32(xo & 0x1FF) << 1)
                | u32(rc);
        }
    
        // A-form: [ op:6 | FRT:5 | FRA:5 | FRB:5 | FRC:5 | XO:5 | Rc:1 ]
        consteval u32 A_form(u8 op, u8 frt, u8 fra, u8 frb, u8 frc, u8 xo, bool rc) {
            return (u32(op) << 26)
                | (u32(frt & 0x1F) << 21)
                | (u32(fra & 0x1F) << 16)
                | (u32(frb & 0x1F) << 11)
                | (u32(frc & 0x1F) << 6)
                | (u32(xo & 0x1F) << 1)
                | u32(rc);
        }
    
        // M-form: [ op:6 | RS:5 | RA:5 | SH:5 | MB:5 | ME:5 | Rc:1 ]
        consteval u32 M_form(u8 op, u8 rs, u8 ra, u8 sh, u8 mb, u8 me, bool rc) {
            return (u32(op) << 26)
                | (u32(rs & 0x1F) << 21)
                | (u32(ra & 0x1F) << 16)
                | (u32(sh & 0x1F) << 11)
                | (u32(mb & 0x1F) << 6)
                | (u32(me & 0x1F) << 1)
                | u32(rc);
        }
        
        consteval u8 cr_field(CR cr) { return u8(cr) << 2; }
    }
        
    // Instructions:

    consteval u32 addi(R rt, R ra, s16 si) { return internal::D_form(14, u8(rt), u8(ra), si); }
    consteval u32 addis(R rt, R ra, s16 si) { return internal::D_form(15, u8(rt), u8(ra), si); }
    consteval u32 add(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 266, rc); }
    consteval u32 mullw(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 235, rc); }

    consteval u32 lwz(R rt, R ra, s16 d) { return internal::D_form(32, u8(rt), u8(ra), d); }
    consteval u32 stw(R rs, R ra, s16 d) { return internal::D_form(36, u8(rs), u8(ra), d); }
    consteval u32 stwu(R rs, R ra, s16 d) { return internal::D_form(37, u8(rs), u8(ra), d); }
    consteval u32 lbz(R rt, R ra, s16 d) { return internal::D_form(34, u8(rt), u8(ra), d); }
    consteval u32 stb(R rs, R ra, s16 d) { return internal::D_form(38, u8(rs), u8(ra), d); }

    consteval u32 ori(R ra, R rs, u16 ui) { return internal::D_form(24, u8(rs), u8(ra), s16(ui)); }
    consteval u32 oris(R ra, R rs, u16 ui) { return internal::D_form(25, u8(rs), u8(ra), s16(ui)); }
    consteval u32 xori(R ra, R rs, u16 ui) { return internal::D_form(26, u8(rs), u8(ra), s16(ui)); }
    consteval u32 andi(R ra, R rs, u16 ui) { return internal::D_form(28, u8(rs), u8(ra), s16(ui)); }
    
    consteval u32 andr(R ra, R rs, R rb, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb),  28, rc); }
    consteval u32 orr(R ra, R rs, R rb, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 444, rc); }
    consteval u32 xorr(R ra, R rs, R rb, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 316, rc); }
    consteval u32 norr(R ra, R rs, R rb, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 124, rc); }
    
    consteval u32 mr(R ra, R rb) { return orr(ra, rb, rb); }
    
    consteval u32 cmpw(R ra, R rb, CR cr = CR::cr0) { return internal::X_form(31, internal::cr_field(cr), u8(ra), u8(rb),  0, false); }
    consteval u32 cmpwi(R ra, s16 simm, CR cr = CR::cr0) { return internal::D_form(11, internal::cr_field(cr), u8(ra), simm); }
    consteval u32 cmplw(R ra, R rb, CR cr = CR::cr0) { return internal::X_form(31, internal::cr_field(cr), u8(ra), u8(rb), 32, false); }
    consteval u32 cmplwi(R ra, u16 uimm, CR cr = CR::cr0) { return internal::D_form(10, internal::cr_field(cr), u8(ra), s16(uimm)); }
    
    consteval u32 li(R rt, s16 si) { return addi(rt, R::r0, si); }
    consteval u32 lis(R rt, s16 si) { return addis(rt, R::r0, si); }
    
    consteval u32 bc(u8 bo, u8 bi, s16 byte_offset) { return internal::B_form(16, bo, bi, byte_offset >> 2, false, false); }
    consteval u32 blt(s16 offset) { return bc(12, 0, offset); }
    consteval u32 bgt(s16 offset) { return bc(12, 1, offset); }
    consteval u32 beq(s16 offset) { return bc(12, 2, offset); }
    consteval u32 bne(s16 offset) { return bc( 4, 2, offset); }

    consteval u32 rlwinm(R ra, R rs, u8 sh, u8 mb, u8 me, bool rc = false) { return internal::M_form(21, u8(rs), u8(ra), sh, mb, me, rc); }
    
    consteval u32 slw(R ra, R rs, R rb, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb),  24, rc); }
    consteval u32 srw(R ra, R rs, R rb, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 536, rc); }
    consteval u32 sraw(R ra, R rs, R rb, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 792, rc); }
    consteval u32 srawi(R ra, R rs, u8 sh, bool rc=false) { return internal::X_form(31, u8(rs), u8(ra), sh, 824, rc); }

    consteval u32 b(s32 relative) { return internal::I_form(18, relative >> 2, false, false); }
    consteval u32 bl(s32 relative) { return internal::I_form(18, relative >> 2, false, true); }
    consteval u32 blr() { return internal::X_form(19, 20, 0, 0, 16, false); }
    consteval u32 nop() { return ori(R::r0, R::r0, 0); }
    
    consteval u32 mfspr(R rt, u16 spr) { return internal::X_form(31, u8(rt), spr & 0x1F, (spr >> 5) & 0x1F, 339, false); }
    consteval u32 mtspr(u16 spr, R rs) { return internal::X_form(31, u8(rs), spr & 0x1F, (spr >> 5) & 0x1F, 467, false); }
    
    consteval u32 mflr(R rt) { return mfspr(rt, 8);  }
    consteval u32 mtlr(R rs) { return mtspr(8,  rs); }

}

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
    stwu r1, -0x10(r1); \
    mflr r2; \
    stw r2, 0x14(r1)

#define tRestoreLR \
    lwz r2, 0x14(r1); \
    mtlr r2; \
    addi r1, r1, 0x10

#endif // TELKIN_REGISTERS
