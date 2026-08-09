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
    using F = FPR;

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
    
        // A-form: [ op:6 | FRT/RT:5 | FRA/RA:5 | FRB/RB:5 | FRC/BC:5 | XO:5 | Rc:1 ]
        consteval u32 A_form(u8 op, u8 frt, u8 fra, u8 frb, u8 frc, u8 xo, bool rc) {
            return (u32(op) << 26)
                | (u32(frt & 0x1F) << 21)
                | (u32(fra & 0x1F) << 16)
                | (u32(frb & 0x1F) << 11)
                | (u32(frc & 0x1F) << 6)
                | (u32(xo & 0x1F) << 1)
                | u32(rc);
        }
    
        // M-form: [ op:6 | RS:5 | RA:5 | SH/RB:5 | MB:5 | ME:5 | Rc:1 ]
        consteval u32 M_form(u8 op, u8 rs, u8 ra, u8 sh, u8 mb, u8 me, bool rc) {
            return (u32(op) << 26)
                | (u32(rs & 0x1F) << 21)
                | (u32(ra & 0x1F) << 16)
                | (u32(sh & 0x1F) << 11)
                | (u32(mb & 0x1F) << 6)
                | (u32(me & 0x1F) << 1)
                | u32(rc);
        }
        
        // XL-form: [ op:6 | BT/BO:5 | BA/BI:5 | BB/BH:5 | XO:10 | LK:1 ]
        consteval u32 XL_form(u8 op, u8 bt, u8 ba, u8 bb, u16 xo, bool lk) {
            return (u32(op) << 26)
                | (u32(bt & 0x1F) << 21)
                | (u32(ba & 0x1F) << 16)
                | (u32(bb & 0x1F) << 11)
                | (u32(xo & 0x3FF) << 1)
                | u32(lk);
        }

        // XFX-form SPR/TBR variant. The two five-bit halves of SPR are encoded swapped.
        consteval u32 XFX_spr_form(u8 op, u8 rsd, u16 spr, u16 xo) {
            return X_form(op, rsd, spr & 0x1F, (spr >> 5) & 0x1F, xo, false);
        }

        // XFX-form CRM variant: [ op:6 | RS:5 | 0:1 | CRM:8 | 0:1 | XO:10 | 0:1 ]
        consteval u32 XFX_crm_form(u8 op, u8 rs, u8 crm, u16 xo) {
            return (u32(op) << 26)
                | (u32(rs & 0x1F) << 21)
                | (u32(crm) << 12)
                | (u32(xo & 0x3FF) << 1);
        }

        // XFL-form: [ op:6 | 0:1 | FM:8 | 0:1 | FRB:5 | XO:10 | Rc:1 ]
        consteval u32 XFL_form(u8 op, u8 fm, u8 frb, u16 xo, bool rc) {
            return (u32(op) << 26)
                | (u32(fm) << 17)
                | (u32(frb & 0x1F) << 11)
                | (u32(xo & 0x3FF) << 1)
                | u32(rc);
        }

        // Paired-single DW-form: [ op:6 | FRT/FRS:5 | RA:5 | W:1 | I:3 | D:12 ]
        consteval u32 DW_form(u8 op, u8 frsd, u8 ra, s16 d, bool w, u8 i) {
            return (u32(op) << 26)
                | (u32(frsd & 0x1F) << 21)
                | (u32(ra & 0x1F) << 16)
                | (u32(w) << 15)
                | (u32(i & 0x7) << 12)
                | u32(d & 0x0FFF);
        }

        // Paired-single XW-form: [ 4:6 | FRT/FRS:5 | RA:5 | RB:5 | W:1 | I:3 | XO:6 | 0:1 ]
        consteval u32 XW_form(u8 xo, u8 frsd, u8 ra, u8 rb, bool w, u8 i) {
            return (u32(4) << 26)
                | (u32(frsd & 0x1F) << 21)
                | (u32(ra & 0x1F) << 16)
                | (u32(rb & 0x1F) << 11)
                | (u32(w) << 10)
                | (u32(i & 0x7) << 7)
                | (u32(xo & 0x3F) << 1);
        }

        consteval u8 cr_field(CR cr) { return (u8(cr) & 0x7) << 2; }

        consteval u32 branch_disp(u32 base, s16 byte_offset) {
            return base | (u32((s32(byte_offset) / 4) & 0x3FFF) << 2);
        }

        consteval u32 branch_bi(u32 base, u8 bi, s16 byte_offset) {
            return branch_disp(base | (u32(bi & 0x1F) << 16), byte_offset);
        }

        consteval u32 branch_cr(u32 base, CR cr, s16 byte_offset) {
            return branch_disp(base | (u32(u8(cr) & 0x7) << 18), byte_offset);
        }

        consteval u32 branch_xl_bi(u32 base, u8 bi) {
            return base | (u32(bi & 0x1F) << 16);
        }

        consteval u32 branch_xl_cr(u32 base, CR cr) {
            return base | (u32(u8(cr) & 0x7) << 18);
        }
    }

    // Integer arithmetic
    consteval u32 addi(R rt, R ra, s16 si) { return internal::D_form(14, u8(rt), u8(ra), si); }
    consteval u32 addis(R rt, R ra, s16 si) { return internal::D_form(15, u8(rt), u8(ra), si); }
    consteval u32 addic(R rt, R ra, s16 si) { return internal::D_form(12, u8(rt), u8(ra), si); }
    consteval u32 addic_dot(R rt, R ra, s16 si) { return internal::D_form(13, u8(rt), u8(ra), si); }
    consteval u32 subfic(R rt, R ra, s16 si) { return internal::D_form(8, u8(rt), u8(ra), si); }
    consteval u32 mulli(R rt, R ra, s16 si) { return internal::D_form(7, u8(rt), u8(ra), si); }
    consteval u32 subi(R rt, R ra, s16 si) { return addi(rt, ra, s16(-s32(si))); }
    consteval u32 subis(R rt, R ra, s16 si) { return addis(rt, ra, s16(-s32(si))); }
    consteval u32 subic(R rt, R ra, s16 si) { return addic(rt, ra, s16(-s32(si))); }
    consteval u32 subic_dot(R rt, R ra, s16 si) { return addic_dot(rt, ra, s16(-s32(si))); }
    consteval u32 la(R rt, R ra, s16 d) { return addi(rt, ra, d); }
    consteval u32 li(R rt, s16 si) { return addi(rt, R::r0, si); }
    consteval u32 lis(R rt, s16 si) { return addis(rt, R::r0, si); }

    consteval u32 add(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 266, rc); }
    consteval u32 addo(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 266, rc); }
    consteval u32 addc(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 10, rc); }
    consteval u32 addco(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 10, rc); }
    consteval u32 subfc(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 8, rc); }
    consteval u32 subfco(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 8, rc); }
    consteval u32 adde(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 138, rc); }
    consteval u32 addeo(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 138, rc); }
    consteval u32 subfe(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 136, rc); }
    consteval u32 subfeo(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 136, rc); }
    consteval u32 subf(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 40, rc); }
    consteval u32 subfo(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 40, rc); }
    consteval u32 mullw(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 235, rc); }
    consteval u32 mullwo(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 235, rc); }
    consteval u32 mulhw(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 75, rc); }
    consteval u32 mulhwu(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 11, rc); }
    consteval u32 divw(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 491, rc); }
    consteval u32 divwo(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 491, rc); }
    consteval u32 divwu(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), false, 459, rc); }
    consteval u32 divwuo(R rt, R ra, R rb, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), u8(rb), true, 459, rc); }
    consteval u32 sub(R rt, R lhs, R rhs, bool rc = false) { return subf(rt, rhs, lhs, rc); }
    consteval u32 subo(R rt, R lhs, R rhs, bool rc = false) { return subfo(rt, rhs, lhs, rc); }
    consteval u32 subc(R rt, R lhs, R rhs, bool rc = false) { return subfc(rt, rhs, lhs, rc); }
    consteval u32 subco(R rt, R lhs, R rhs, bool rc = false) { return subfco(rt, rhs, lhs, rc); }
    consteval u32 addme(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, false, 234, rc); }
    consteval u32 addmeo(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, true, 234, rc); }
    consteval u32 subfme(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, false, 232, rc); }
    consteval u32 subfmeo(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, true, 232, rc); }
    consteval u32 addze(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, false, 202, rc); }
    consteval u32 addzeo(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, true, 202, rc); }
    consteval u32 subfze(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, false, 200, rc); }
    consteval u32 subfzeo(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, true, 200, rc); }
    consteval u32 neg(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, false, 104, rc); }
    consteval u32 nego(R rt, R ra, bool rc = false) { return internal::XO_form(31, u8(rt), u8(ra), 0, true, 104, rc); }

    // Integer compare
    consteval u32 cmpi(R ra, s16 si, CR cr = CR::cr0) { return internal::D_form(11, internal::cr_field(cr), u8(ra), si); }
    consteval u32 cmpli(R ra, u16 ui, CR cr = CR::cr0) { return internal::D_form(10, internal::cr_field(cr), u8(ra), s16(ui)); }
    consteval u32 cmp(R ra, R rb, CR cr = CR::cr0) { return internal::X_form(31, internal::cr_field(cr), u8(ra), u8(rb), 0, false); }
    consteval u32 cmpl(R ra, R rb, CR cr = CR::cr0) { return internal::X_form(31, internal::cr_field(cr), u8(ra), u8(rb), 32, false); }
    consteval u32 cmpwi(R ra, s16 si, CR cr = CR::cr0) { return cmpi(ra, si, cr); }
    consteval u32 cmplwi(R ra, u16 ui, CR cr = CR::cr0) { return cmpli(ra, ui, cr); }
    consteval u32 cmpw(R ra, R rb, CR cr = CR::cr0) { return cmp(ra, rb, cr); }
    consteval u32 cmplw(R ra, R rb, CR cr = CR::cr0) { return cmpl(ra, rb, cr); }

    // Integer logical
    consteval u32 andi(R ra, R rs, u16 ui) { return internal::D_form(28, u8(rs), u8(ra), s16(ui)); }
    consteval u32 andis(R ra, R rs, u16 ui) { return internal::D_form(29, u8(rs), u8(ra), s16(ui)); }
    consteval u32 ori(R ra, R rs, u16 ui) { return internal::D_form(24, u8(rs), u8(ra), s16(ui)); }
    consteval u32 oris(R ra, R rs, u16 ui) { return internal::D_form(25, u8(rs), u8(ra), s16(ui)); }
    consteval u32 xori(R ra, R rs, u16 ui) { return internal::D_form(26, u8(rs), u8(ra), s16(ui)); }
    consteval u32 xoris(R ra, R rs, u16 ui) { return internal::D_form(27, u8(rs), u8(ra), s16(ui)); }
    consteval u32 andr(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 28, rc); }
    consteval u32 xorr(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 316, rc); }
    consteval u32 orr(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 444, rc); }
    consteval u32 nand(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 476, rc); }
    consteval u32 norr(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 124, rc); }
    consteval u32 andc(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 60, rc); }
    consteval u32 eqv(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 284, rc); }
    consteval u32 orc(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 412, rc); }
    consteval u32 extsb(R ra, R rs, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), 0, 954, rc); }
    consteval u32 extsh(R ra, R rs, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), 0, 922, rc); }
    consteval u32 cntlzw(R ra, R rs, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), 0, 26, rc); }
    consteval u32 mr(R ra, R rs, bool rc = false) { return orr(ra, rs, rs, rc); }
    consteval u32 notr(R ra, R rs, bool rc = false) { return norr(ra, rs, rs, rc); }
    consteval u32 isel(R rt, R ra, R rb, u8 bc) { return internal::A_form(31, u8(rt), u8(ra), u8(rb), bc, 15, false); }
    consteval u32 isellt(R rt, R ra, R rb) { return isel(rt, ra, rb, 0); }
    consteval u32 iselgt(R rt, R ra, R rb) { return isel(rt, ra, rb, 1); }
    consteval u32 iseleq(R rt, R ra, R rb) { return isel(rt, ra, rb, 2); }
    consteval u32 yield() { return 0x7F7BDB78u; }
    consteval u32 mdoio() { return 0x7FBDEB78u; }
    consteval u32 mdoom() { return 0x7FDEF378u; }
    consteval u32 nop() { return ori(R::r0, R::r0, 0); }

    // Integer rotate and shift
    consteval u32 rlwinm(R ra, R rs, u8 sh, u8 mb, u8 me, bool rc = false) { return internal::M_form(21, u8(rs), u8(ra), sh, mb, me, rc); }
    consteval u32 rlwnm(R ra, R rs, R rb, u8 mb, u8 me, bool rc = false) { return internal::M_form(23, u8(rs), u8(ra), u8(rb), mb, me, rc); }
    consteval u32 rlwimi(R ra, R rs, u8 sh, u8 mb, u8 me, bool rc = false) { return internal::M_form(20, u8(rs), u8(ra), sh, mb, me, rc); }
    consteval u32 extlwi(R ra, R rs, u8 n, u8 b, bool rc = false) { return rlwinm(ra, rs, b, 0, u8(n - 1), rc); }
    consteval u32 extrwi(R ra, R rs, u8 n, u8 b, bool rc = false) { return rlwinm(ra, rs, u8(b + n), u8(32 - n), 31, rc); }
    consteval u32 inslwi(R ra, R rs, u8 n, u8 b, bool rc = false) { return rlwimi(ra, rs, u8(32 - b), b, u8(b + n - 1), rc); }
    consteval u32 insrwi(R ra, R rs, u8 n, u8 b, bool rc = false) { return rlwimi(ra, rs, u8(32 - (b + n)), b, u8(b + n - 1), rc); }
    consteval u32 rotlwi(R ra, R rs, u8 n, bool rc = false) { return rlwinm(ra, rs, n, 0, 31, rc); }
    consteval u32 rotrwi(R ra, R rs, u8 n, bool rc = false) { return rlwinm(ra, rs, u8(32 - n), 0, 31, rc); }
    consteval u32 rotlw(R ra, R rs, R rb, bool rc = false) { return rlwnm(ra, rs, rb, 0, 31, rc); }
    consteval u32 slwi(R ra, R rs, u8 n, bool rc = false) { return rlwinm(ra, rs, n, 0, u8(31 - n), rc); }
    consteval u32 srwi(R ra, R rs, u8 n, bool rc = false) { return rlwinm(ra, rs, u8(32 - n), n, 31, rc); }
    consteval u32 clrlwi(R ra, R rs, u8 n, bool rc = false) { return rlwinm(ra, rs, 0, n, 31, rc); }
    consteval u32 clrrwi(R ra, R rs, u8 n, bool rc = false) { return rlwinm(ra, rs, 0, 0, u8(31 - n), rc); }
    consteval u32 clrlslwi(R ra, R rs, u8 b, u8 n, bool rc = false) { return rlwinm(ra, rs, n, u8(b - n), u8(31 - n), rc); }
    consteval u32 slw(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 24, rc); }
    consteval u32 srw(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 536, rc); }
    consteval u32 srawi(R ra, R rs, u8 sh, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), sh, 824, rc); }
    consteval u32 sraw(R ra, R rs, R rb, bool rc = false) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 792, rc); }

    // Floating-point arithmetic
    consteval u32 fadd(F fd, F fa, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), 0, 21, rc); }
    consteval u32 fadds(F fd, F fa, F fb, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), u8(fb), 0, 21, rc); }
    consteval u32 fsub(F fd, F fa, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), 0, 20, rc); }
    consteval u32 fsubs(F fd, F fa, F fb, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), u8(fb), 0, 20, rc); }
    consteval u32 fmul(F fd, F fa, F fc, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), 0, u8(fc), 25, rc); }
    consteval u32 fmuls(F fd, F fa, F fc, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), 0, u8(fc), 25, rc); }
    consteval u32 fdiv(F fd, F fa, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), 0, 18, rc); }
    consteval u32 fdivs(F fd, F fa, F fb, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), u8(fb), 0, 18, rc); }
    consteval u32 fmadd(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), u8(fc), 29, rc); }
    consteval u32 fmadds(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), u8(fb), u8(fc), 29, rc); }
    consteval u32 fmsub(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), u8(fc), 28, rc); }
    consteval u32 fmsubs(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), u8(fb), u8(fc), 28, rc); }
    consteval u32 fnmadd(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), u8(fc), 31, rc); }
    consteval u32 fnmadds(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), u8(fb), u8(fc), 31, rc); }
    consteval u32 fnmsub(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), u8(fc), 30, rc); }
    consteval u32 fnmsubs(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(59, u8(fd), u8(fa), u8(fb), u8(fc), 30, rc); }
    consteval u32 frsp(F fd, F fb, bool rc = false) { return internal::X_form(63, u8(fd), 0, u8(fb), 12, rc); }
    consteval u32 fctiw(F fd, F fb, bool rc = false) { return internal::X_form(63, u8(fd), 0, u8(fb), 14, rc); }
    consteval u32 fctiwz(F fd, F fb, bool rc = false) { return internal::X_form(63, u8(fd), 0, u8(fb), 15, rc); }
    consteval u32 fcmpu(CR cr, F fa, F fb) { return internal::X_form(63, internal::cr_field(cr), u8(fa), u8(fb), 0, false); }
    consteval u32 fcmpo(CR cr, F fa, F fb) { return internal::X_form(63, internal::cr_field(cr), u8(fa), u8(fb), 32, false); }
    consteval u32 mffs(F fd, bool rc = false) { return internal::X_form(63, u8(fd), 0, 0, 583, rc); }
    consteval u32 mcrfs(CR crd, CR crs) { return internal::X_form(63, internal::cr_field(crd), internal::cr_field(crs), 0, 64, false); }
    consteval u32 mtfsfi(CR crd, u8 imm4, bool rc = false) { return 0xFC00010Cu | (u32(internal::cr_field(crd)) << 21) | (u32(imm4 & 0xF) << 12) | u32(rc); }
    consteval u32 mtfsf(u8 fm, F fb, bool rc = false) { return internal::XFL_form(63, fm, u8(fb), 711, rc); }
    consteval u32 mtfsb0(u8 crb, bool rc = false) { return internal::X_form(63, crb, 0, 0, 70, rc); }
    consteval u32 mtfsb1(u8 crb, bool rc = false) { return internal::X_form(63, crb, 0, 0, 38, rc); }

    // Integer loads and stores
    consteval u32 lbz(R rt, R ra, s16 d) { return internal::D_form(34, u8(rt), u8(ra), d); }
    consteval u32 lbzu(R rt, R ra, s16 d) { return internal::D_form(35, u8(rt), u8(ra), d); }
    consteval u32 lhz(R rt, R ra, s16 d) { return internal::D_form(40, u8(rt), u8(ra), d); }
    consteval u32 lhzu(R rt, R ra, s16 d) { return internal::D_form(41, u8(rt), u8(ra), d); }
    consteval u32 lha(R rt, R ra, s16 d) { return internal::D_form(42, u8(rt), u8(ra), d); }
    consteval u32 lhau(R rt, R ra, s16 d) { return internal::D_form(43, u8(rt), u8(ra), d); }
    consteval u32 lwz(R rt, R ra, s16 d) { return internal::D_form(32, u8(rt), u8(ra), d); }
    consteval u32 lwzu(R rt, R ra, s16 d) { return internal::D_form(33, u8(rt), u8(ra), d); }
    consteval u32 lbzx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 87, false); }
    consteval u32 lbzux(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 119, false); }
    consteval u32 lhzx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 279, false); }
    consteval u32 lhzux(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 311, false); }
    consteval u32 lhax(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 343, false); }
    consteval u32 lhaux(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 375, false); }
    consteval u32 lwzx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 23, false); }
    consteval u32 lwzux(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 55, false); }
    consteval u32 stb(R rs, R ra, s16 d) { return internal::D_form(38, u8(rs), u8(ra), d); }
    consteval u32 stbu(R rs, R ra, s16 d) { return internal::D_form(39, u8(rs), u8(ra), d); }
    consteval u32 sth(R rs, R ra, s16 d) { return internal::D_form(44, u8(rs), u8(ra), d); }
    consteval u32 sthu(R rs, R ra, s16 d) { return internal::D_form(45, u8(rs), u8(ra), d); }
    consteval u32 stw(R rs, R ra, s16 d) { return internal::D_form(36, u8(rs), u8(ra), d); }
    consteval u32 stwu(R rs, R ra, s16 d) { return internal::D_form(37, u8(rs), u8(ra), d); }
    consteval u32 stbx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 215, false); }
    consteval u32 stbux(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 247, false); }
    consteval u32 sthx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 407, false); }
    consteval u32 sthux(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 439, false); }
    consteval u32 stwx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 151, false); }
    consteval u32 stwux(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 183, false); }
    consteval u32 lhbrx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 790, false); }
    consteval u32 lwbrx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 534, false); }
    consteval u32 sthbrx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 918, false); }
    consteval u32 stwbrx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 662, false); }
    consteval u32 lmw(R rt, R ra, s16 d) { return internal::D_form(46, u8(rt), u8(ra), d); }
    consteval u32 stmw(R rs, R ra, s16 d) { return internal::D_form(47, u8(rs), u8(ra), d); }
    consteval u32 lswi(R rt, R ra, u8 nb) { return internal::X_form(31, u8(rt), u8(ra), nb, 597, false); }
    consteval u32 lswx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 533, false); }
    consteval u32 stswi(R rs, R ra, u8 nb) { return internal::X_form(31, u8(rs), u8(ra), nb, 725, false); }
    consteval u32 stswx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 661, false); }

    // Memory synchronization
    consteval u32 sync(u8 l = 0) { return internal::X_form(31, l, 0, 0, 598, false); }
    consteval u32 lwsync() { return sync(1); }
    consteval u32 ptesync() { return sync(2); }
    consteval u32 isync() { return internal::XL_form(19, 0, 0, 0, 150, false); }
    consteval u32 eieio() { return internal::X_form(31, 0, 0, 0, 854, false); }
    consteval u32 lwarx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 20, false); }
    consteval u32 stwcx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 150, true); }

    // Floating-point loads and stores
    consteval u32 lfs(F ft, R ra, s16 d) { return internal::D_form(48, u8(ft), u8(ra), d); }
    consteval u32 lfsu(F ft, R ra, s16 d) { return internal::D_form(49, u8(ft), u8(ra), d); }
    consteval u32 lfd(F ft, R ra, s16 d) { return internal::D_form(50, u8(ft), u8(ra), d); }
    consteval u32 lfdu(F ft, R ra, s16 d) { return internal::D_form(51, u8(ft), u8(ra), d); }
    consteval u32 lfsx(F ft, R ra, R rb) { return internal::X_form(31, u8(ft), u8(ra), u8(rb), 535, false); }
    consteval u32 lfsux(F ft, R ra, R rb) { return internal::X_form(31, u8(ft), u8(ra), u8(rb), 567, false); }
    consteval u32 lfdx(F ft, R ra, R rb) { return internal::X_form(31, u8(ft), u8(ra), u8(rb), 599, false); }
    consteval u32 lfdux(F ft, R ra, R rb) { return internal::X_form(31, u8(ft), u8(ra), u8(rb), 631, false); }
    consteval u32 stfs(F fs, R ra, s16 d) { return internal::D_form(52, u8(fs), u8(ra), d); }
    consteval u32 stfsu(F fs, R ra, s16 d) { return internal::D_form(53, u8(fs), u8(ra), d); }
    consteval u32 stfd(F fs, R ra, s16 d) { return internal::D_form(54, u8(fs), u8(ra), d); }
    consteval u32 stfdu(F fs, R ra, s16 d) { return internal::D_form(55, u8(fs), u8(ra), d); }
    consteval u32 stfsx(F fs, R ra, R rb) { return internal::X_form(31, u8(fs), u8(ra), u8(rb), 663, false); }
    consteval u32 stfsux(F fs, R ra, R rb) { return internal::X_form(31, u8(fs), u8(ra), u8(rb), 695, false); }
    consteval u32 stfdx(F fs, R ra, R rb) { return internal::X_form(31, u8(fs), u8(ra), u8(rb), 727, false); }
    consteval u32 stfdux(F fs, R ra, R rb) { return internal::X_form(31, u8(fs), u8(ra), u8(rb), 759, false); }
    consteval u32 fmr(F fd, F fb, bool rc = false) { return internal::X_form(63, u8(fd), 0, u8(fb), 72, rc); }
    consteval u32 fabs(F fd, F fb, bool rc = false) { return internal::X_form(63, u8(fd), 0, u8(fb), 264, rc); }
    consteval u32 fneg(F fd, F fb, bool rc = false) { return internal::X_form(63, u8(fd), 0, u8(fb), 40, rc); }
    consteval u32 fnabs(F fd, F fb, bool rc = false) { return internal::X_form(63, u8(fd), 0, u8(fb), 136, rc); }

    // Branch instructions
    consteval u32 bt(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41800000u, bi, byte_offset); }
    consteval u32 bf(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40800000u, bi, byte_offset); }
    consteval u32 bdnz(s16 byte_offset) { return internal::branch_disp(0x42000000u, byte_offset); }
    consteval u32 bdnzt(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41000000u, bi, byte_offset); }
    consteval u32 bdnzf(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40000000u, bi, byte_offset); }
    consteval u32 bdz(s16 byte_offset) { return internal::branch_disp(0x42400000u, byte_offset); }
    consteval u32 bdzt(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41400000u, bi, byte_offset); }
    consteval u32 bdzf(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40400000u, bi, byte_offset); }
    consteval u32 bta(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41800002u, bi, byte_offset); }
    consteval u32 bfa(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40800002u, bi, byte_offset); }
    consteval u32 bdnza(s16 byte_offset) { return internal::branch_disp(0x42000002u, byte_offset); }
    consteval u32 bdnzta(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41000002u, bi, byte_offset); }
    consteval u32 bdnzfa(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40000002u, bi, byte_offset); }
    consteval u32 bdza(s16 byte_offset) { return internal::branch_disp(0x42400002u, byte_offset); }
    consteval u32 bdzta(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41400002u, bi, byte_offset); }
    consteval u32 bdzfa(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40400002u, bi, byte_offset); }
    consteval u32 btl(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41800001u, bi, byte_offset); }
    consteval u32 bfl(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40800001u, bi, byte_offset); }
    consteval u32 bdnzl(s16 byte_offset) { return internal::branch_disp(0x42000001u, byte_offset); }
    consteval u32 bdnztl(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41000001u, bi, byte_offset); }
    consteval u32 bdnzfl(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40000001u, bi, byte_offset); }
    consteval u32 bdzl(s16 byte_offset) { return internal::branch_disp(0x42400001u, byte_offset); }
    consteval u32 bdztl(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41400001u, bi, byte_offset); }
    consteval u32 bdzfl(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40400001u, bi, byte_offset); }
    consteval u32 btla(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41800003u, bi, byte_offset); }
    consteval u32 bfla(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40800003u, bi, byte_offset); }
    consteval u32 bdnzla(s16 byte_offset) { return internal::branch_disp(0x42000003u, byte_offset); }
    consteval u32 bdnztla(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41000003u, bi, byte_offset); }
    consteval u32 bdnzfla(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40000003u, bi, byte_offset); }
    consteval u32 bdzla(s16 byte_offset) { return internal::branch_disp(0x42400003u, byte_offset); }
    consteval u32 bdztla(u8 bi, s16 byte_offset) { return internal::branch_bi(0x41400003u, bi, byte_offset); }
    consteval u32 bdzfla(u8 bi, s16 byte_offset) { return internal::branch_bi(0x40400003u, bi, byte_offset); }
    consteval u32 btlr(u8 bi) { return internal::branch_xl_bi(0x4D800020u, bi); }
    consteval u32 bflr(u8 bi) { return internal::branch_xl_bi(0x4C800020u, bi); }
    consteval u32 bdnzlr() { return 0x4E000020u; }
    consteval u32 bdnztlr(u8 bi) { return internal::branch_xl_bi(0x4D000020u, bi); }
    consteval u32 bdnzflr(u8 bi) { return internal::branch_xl_bi(0x4C000020u, bi); }
    consteval u32 bdzlr() { return 0x4E400020u; }
    consteval u32 bdztlr(u8 bi) { return internal::branch_xl_bi(0x4D400020u, bi); }
    consteval u32 bdzflr(u8 bi) { return internal::branch_xl_bi(0x4C400020u, bi); }
    consteval u32 btctr(u8 bi) { return internal::branch_xl_bi(0x4D800420u, bi); }
    consteval u32 bfctr(u8 bi) { return internal::branch_xl_bi(0x4C800420u, bi); }
    consteval u32 btlrl(u8 bi) { return internal::branch_xl_bi(0x4D800021u, bi); }
    consteval u32 bflrl(u8 bi) { return internal::branch_xl_bi(0x4C800021u, bi); }
    consteval u32 bdnzlrl() { return 0x4E000021u; }
    consteval u32 bdnztlrl(u8 bi) { return internal::branch_xl_bi(0x4D000021u, bi); }
    consteval u32 bdnzflrl(u8 bi) { return internal::branch_xl_bi(0x4C000021u, bi); }
    consteval u32 bdzlrl() { return 0x4E400021u; }
    consteval u32 bdztlrl(u8 bi) { return internal::branch_xl_bi(0x4D400021u, bi); }
    consteval u32 bdzflrl(u8 bi) { return internal::branch_xl_bi(0x4C400021u, bi); }
    consteval u32 btctrl(u8 bi) { return internal::branch_xl_bi(0x4D800421u, bi); }
    consteval u32 bfctrl(u8 bi) { return internal::branch_xl_bi(0x4C800421u, bi); }
    consteval u32 bltlr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D800020u, cr); }
    consteval u32 blelr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810020u, cr); }
    consteval u32 beqlr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D820020u, cr); }
    consteval u32 bgelr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800020u, cr); }
    consteval u32 bgtlr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D810020u, cr); }
    consteval u32 bnllr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800020u, cr); }
    consteval u32 bnelr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C820020u, cr); }
    consteval u32 bnglr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810020u, cr); }
    consteval u32 bsolr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830020u, cr); }
    consteval u32 bnslr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830020u, cr); }
    consteval u32 bunlr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830020u, cr); }
    consteval u32 bnulr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830020u, cr); }
    consteval u32 bltctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D800420u, cr); }
    consteval u32 blectr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810420u, cr); }
    consteval u32 beqctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D820420u, cr); }
    consteval u32 bgectr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800420u, cr); }
    consteval u32 bgtctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D810420u, cr); }
    consteval u32 bnlctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800420u, cr); }
    consteval u32 bnectr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C820420u, cr); }
    consteval u32 bngctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810420u, cr); }
    consteval u32 bsoctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830420u, cr); }
    consteval u32 bnsctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830420u, cr); }
    consteval u32 bunctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830420u, cr); }
    consteval u32 bnuctr(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830420u, cr); }
    consteval u32 bltlrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D800021u, cr); }
    consteval u32 blelrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810021u, cr); }
    consteval u32 beqlrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D820021u, cr); }
    consteval u32 bgelrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800021u, cr); }
    consteval u32 bgtlrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D810021u, cr); }
    consteval u32 bnllrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800021u, cr); }
    consteval u32 bnelrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C820021u, cr); }
    consteval u32 bnglrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810021u, cr); }
    consteval u32 bsolrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830021u, cr); }
    consteval u32 bnslrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830021u, cr); }
    consteval u32 bunlrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830021u, cr); }
    consteval u32 bnulrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830021u, cr); }
    consteval u32 bltctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D800421u, cr); }
    consteval u32 blectrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810421u, cr); }
    consteval u32 beqctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D820421u, cr); }
    consteval u32 bgectrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800421u, cr); }
    consteval u32 bgtctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D810421u, cr); }
    consteval u32 bnlctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C800421u, cr); }
    consteval u32 bnectrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C820421u, cr); }
    consteval u32 bngctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C810421u, cr); }
    consteval u32 bsoctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830421u, cr); }
    consteval u32 bnsctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830421u, cr); }
    consteval u32 bunctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4D830421u, cr); }
    consteval u32 bnuctrl(CR cr = CR::cr0) { return internal::branch_xl_cr(0x4C830421u, cr); }
    consteval u32 b(s32 byte_offset) { return internal::I_form(18, byte_offset / 4, false, false); }
    consteval u32 ba(s32 byte_offset) { return internal::I_form(18, byte_offset / 4, true, false); }
    consteval u32 bl(s32 byte_offset) { return internal::I_form(18, byte_offset / 4, false, true); }
    consteval u32 bla(s32 byte_offset) { return internal::I_form(18, byte_offset / 4, true, true); }
    consteval u32 blr() { return 0x4E800020u; }
    consteval u32 bctr() { return 0x4E800420u; }
    consteval u32 blrl() { return 0x4E800021u; }
    consteval u32 bctrl() { return 0x4E800421u; }
    consteval u32 bclr(u8 bo, u8 bi) { return internal::XL_form(19, bo, bi, 0, 16, false); }
    consteval u32 bclrl(u8 bo, u8 bi) { return internal::XL_form(19, bo, bi, 0, 16, true); }
    consteval u32 bcctr(u8 bo, u8 bi) { return internal::XL_form(19, bo, bi, 0, 528, false); }
    consteval u32 bcctrl(u8 bo, u8 bi) { return internal::XL_form(19, bo, bi, 0, 528, true); }
    consteval u32 bc(u8 bo, u8 bi, s16 byte_offset) { return internal::B_form(16, bo, bi, s16(s32(byte_offset) / 4), false, false); }
    consteval u32 bca(u8 bo, u8 bi, s16 byte_offset) { return internal::B_form(16, bo, bi, s16(s32(byte_offset) / 4), true, false); }
    consteval u32 bcl(u8 bo, u8 bi, s16 byte_offset) { return internal::B_form(16, bo, bi, s16(s32(byte_offset) / 4), false, true); }
    consteval u32 bcla(u8 bo, u8 bi, s16 byte_offset) { return internal::B_form(16, bo, bi, s16(s32(byte_offset) / 4), true, true); }
    consteval u32 blt(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41800000u, cr, byte_offset); }
    consteval u32 ble(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810000u, cr, byte_offset); }
    consteval u32 beq(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41820000u, cr, byte_offset); }
    consteval u32 bge(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800000u, cr, byte_offset); }
    consteval u32 bgt(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41810000u, cr, byte_offset); }
    consteval u32 bnl(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800000u, cr, byte_offset); }
    consteval u32 bne(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40820000u, cr, byte_offset); }
    consteval u32 bng(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810000u, cr, byte_offset); }
    consteval u32 bso(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830000u, cr, byte_offset); }
    consteval u32 bns(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830000u, cr, byte_offset); }
    consteval u32 bun(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830000u, cr, byte_offset); }
    consteval u32 bnu(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830000u, cr, byte_offset); }
    consteval u32 blta(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41800002u, cr, byte_offset); }
    consteval u32 blea(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810002u, cr, byte_offset); }
    consteval u32 beqa(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41820002u, cr, byte_offset); }
    consteval u32 bgea(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800002u, cr, byte_offset); }
    consteval u32 bgta(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41810002u, cr, byte_offset); }
    consteval u32 bnla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800002u, cr, byte_offset); }
    consteval u32 bnea(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40820002u, cr, byte_offset); }
    consteval u32 bnga(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810002u, cr, byte_offset); }
    consteval u32 bsoa(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830002u, cr, byte_offset); }
    consteval u32 bnsa(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830002u, cr, byte_offset); }
    consteval u32 buna(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830002u, cr, byte_offset); }
    consteval u32 bnua(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830002u, cr, byte_offset); }
    consteval u32 bltl(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41800001u, cr, byte_offset); }
    consteval u32 blel(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810001u, cr, byte_offset); }
    consteval u32 beql(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41820001u, cr, byte_offset); }
    consteval u32 bgel(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800001u, cr, byte_offset); }
    consteval u32 bgtl(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41810001u, cr, byte_offset); }
    consteval u32 bnll(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800001u, cr, byte_offset); }
    consteval u32 bnel(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40820001u, cr, byte_offset); }
    consteval u32 bngl(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810001u, cr, byte_offset); }
    consteval u32 bsol(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830001u, cr, byte_offset); }
    consteval u32 bnsl(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830001u, cr, byte_offset); }
    consteval u32 bunl(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830001u, cr, byte_offset); }
    consteval u32 bnul(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830001u, cr, byte_offset); }
    consteval u32 bltla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41800003u, cr, byte_offset); }
    consteval u32 blela(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810003u, cr, byte_offset); }
    consteval u32 beqla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41820003u, cr, byte_offset); }
    consteval u32 bgela(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800003u, cr, byte_offset); }
    consteval u32 bgtla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41810003u, cr, byte_offset); }
    consteval u32 bnlla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40800003u, cr, byte_offset); }
    consteval u32 bnela(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40820003u, cr, byte_offset); }
    consteval u32 bngla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40810003u, cr, byte_offset); }
    consteval u32 bsola(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830003u, cr, byte_offset); }
    consteval u32 bnsla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830003u, cr, byte_offset); }
    consteval u32 bunla(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x41830003u, cr, byte_offset); }
    consteval u32 bnula(s16 byte_offset, CR cr = CR::cr0) { return internal::branch_cr(0x40830003u, cr, byte_offset); }


    // Condition-register
    consteval u32 mcrf(CR crd, CR crs) { return internal::XL_form(19, internal::cr_field(crd), internal::cr_field(crs), 0, 0, false); }
    consteval u32 crand(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 257, false); }
    consteval u32 crxor(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 193, false); }
    consteval u32 cror(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 449, false); }
    consteval u32 crnand(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 225, false); }
    consteval u32 crnor(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 33, false); }
    consteval u32 crandc(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 129, false); }
    consteval u32 creqv(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 289, false); }
    consteval u32 crorc(u8 crbd, u8 crba, u8 crbb) { return internal::XL_form(19, crbd, crba, crbb, 417, false); }
    consteval u32 crset(u8 crbd) { return creqv(crbd, crbd, crbd); }
    consteval u32 crclr(u8 crbd) { return crxor(crbd, crbd, crbd); }
    consteval u32 crmove(u8 crbd, u8 crba) { return cror(crbd, crba, crba); }
    consteval u32 crnot(u8 crbd, u8 crba) { return crnor(crbd, crba, crba); }

    // System
    consteval u32 sc() { return 0x44000002u; }
    consteval u32 rfi() { return 0x4C000064u; }

    // Trap instructions
    consteval u32 twi(u8 to, R ra, s16 si) { return internal::D_form(3, to, u8(ra), si); }
    consteval u32 tw(u8 to, R ra, R rb) { return internal::X_form(31, to, u8(ra), u8(rb), 4, false); }
    consteval u32 twlti(R ra, s16 imm) { return 0x0E000000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twlei(R ra, s16 imm) { return 0x0E800000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 tweqi(R ra, s16 imm) { return 0x0C800000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twgei(R ra, s16 imm) { return 0x0D800000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twgti(R ra, s16 imm) { return 0x0D000000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twnli(R ra, s16 imm) { return 0x0D800000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twnei(R ra, s16 imm) { return 0x0F000000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twngi(R ra, s16 imm) { return 0x0E800000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twllti(R ra, u16 imm) { return 0x0C400000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twllei(R ra, u16 imm) { return 0x0CC00000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twlgei(R ra, u16 imm) { return 0x0CA00000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twlgti(R ra, u16 imm) { return 0x0C200000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twlnli(R ra, u16 imm) { return 0x0CA00000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 twlngi(R ra, u16 imm) { return 0x0CC00000u | (u32(u8(ra)) << 16) | u16(imm); }
    consteval u32 trap() { return 0x7FE00008u; }
    consteval u32 twlt(R ra, R rb) { return 0x7E000008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twle(R ra, R rb) { return 0x7E800008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 tweq(R ra, R rb) { return 0x7C800008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twge(R ra, R rb) { return 0x7D800008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twgt(R ra, R rb) { return 0x7D000008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twnl(R ra, R rb) { return 0x7D800008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twne(R ra, R rb) { return 0x7F000008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twng(R ra, R rb) { return 0x7E800008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twllt(R ra, R rb) { return 0x7C400008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twlle(R ra, R rb) { return 0x7CC00008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twlge(R ra, R rb) { return 0x7CA00008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twlgt(R ra, R rb) { return 0x7C200008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twlnl(R ra, R rb) { return 0x7CA00008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }
    consteval u32 twlng(R ra, R rb) { return 0x7CC00008u | (u32(u8(ra)) << 16) | (u32(u8(rb)) << 11); }


    // Special register ops
    consteval u32 mtmsr(R rs) { return internal::X_form(31, u8(rs), 0, 0, 146, false); }
    consteval u32 mfmsr(R rt) { return internal::X_form(31, u8(rt), 0, 0, 83, false); }
    consteval u32 mtspr(u16 spr, R rs) { return internal::XFX_spr_form(31, u8(rs), spr, 467); }
    consteval u32 mfspr(R rt, u16 spr) { return internal::XFX_spr_form(31, u8(rt), spr, 339); }
    consteval u32 mtlr(R rs) { return mtspr(8, rs); }
    consteval u32 mflr(R rt) { return mfspr(rt, 8); }
    consteval u32 mtctr(R rs) { return mtspr(9, rs); }
    consteval u32 mfctr(R rt) { return mfspr(rt, 9); }
    consteval u32 mtxer(R rs) { return mtspr(1, rs); }
    consteval u32 mfxer(R rt) { return mfspr(rt, 1); }
    consteval u32 mfpvr(R rt) { return mfspr(rt, 287); }
    consteval u32 mtdsisr(R rs) { return mtspr(18, rs); }
    consteval u32 mfdsisr(R rt) { return mfspr(rt, 18); }
    consteval u32 mtdar(R rs) { return mtspr(19, rs); }
    consteval u32 mfdar(R rt) { return mfspr(rt, 19); }
    consteval u32 mtdec(R rs) { return mtspr(22, rs); }
    consteval u32 mfdec(R rt) { return mfspr(rt, 22); }
    consteval u32 mtsrr0(R rs) { return mtspr(26, rs); }
    consteval u32 mfsrr0(R rt) { return mfspr(rt, 26); }
    consteval u32 mtsrr1(R rs) { return mtspr(27, rs); }
    consteval u32 mfsrr1(R rt) { return mfspr(rt, 27); }
    consteval u32 mtsdr1(R rs) { return mtspr(25, rs); }
    consteval u32 mfsdr1(R rt) { return mfspr(rt, 25); }
    consteval u32 mtear(R rs) { return mtspr(282, rs); }
    consteval u32 mfear(R rt) { return mfspr(rt, 282); }
    consteval u32 mtsprg(u8 sprg, R rs) { return mtspr(u16(272 + (sprg & 3)), rs); }
    consteval u32 mfsprg(R rt, u8 sprg) { return mfspr(rt, u16(272 + (sprg & 3))); }
    consteval u32 mtibatl(u8 bat, R rs) { return mtspr(u16(529 + ((bat & 3) * 2)), rs); }
    consteval u32 mtibatu(u8 bat, R rs) { return mtspr(u16(528 + ((bat & 3) * 2)), rs); }
    consteval u32 mfibatl(R rt, u8 bat) { return mfspr(rt, u16(529 + ((bat & 3) * 2))); }
    consteval u32 mfibatu(R rt, u8 bat) { return mfspr(rt, u16(528 + ((bat & 3) * 2))); }
    consteval u32 mtdbatl(u8 bat, R rs) { return mtspr(u16(537 + ((bat & 3) * 2)), rs); }
    consteval u32 mtdbatu(u8 bat, R rs) { return mtspr(u16(536 + ((bat & 3) * 2)), rs); }
    consteval u32 mfdbatl(R rt, u8 bat) { return mfspr(rt, u16(537 + ((bat & 3) * 2))); }
    consteval u32 mfdbatu(R rt, u8 bat) { return mfspr(rt, u16(536 + ((bat & 3) * 2))); }
    consteval u32 mttbl(R rs) { return mtspr(284, rs); }
    consteval u32 mttbu(R rs) { return mtspr(285, rs); }
    consteval u32 mftbl(R rt) { return mfspr(rt, 268); }
    consteval u32 mftbu(R rt) { return mfspr(rt, 269); }
    consteval u32 mtcrf(u8 crm, R rs) { return internal::XFX_crm_form(31, u8(rs), crm, 144); }
    consteval u32 mtcr(R rs) { return mtcrf(0xFF, rs); }
    consteval u32 mfcr(R rt) { return internal::X_form(31, u8(rt), 0, 0, 19, false); }
    consteval u32 mcrxr(CR crd) { return internal::X_form(31, internal::cr_field(crd), 0, 0, 512, false); }

    // Cache management
    consteval u32 dcbz(R ra, R rb) { return internal::X_form(31, 0, u8(ra), u8(rb), 1014, false); }
    consteval u32 dcbst(R ra, R rb) { return internal::X_form(31, 0, u8(ra), u8(rb), 54, false); }
    consteval u32 dcbf(R ra, R rb) { return internal::X_form(31, 0, u8(ra), u8(rb), 86, false); }
    consteval u32 dcbi(R ra, R rb) { return internal::X_form(31, 0, u8(ra), u8(rb), 470, false); }
    consteval u32 icbi(R ra, R rb) { return internal::X_form(31, 0, u8(ra), u8(rb), 982, false); }

    // Segment-register manipulation
    consteval u32 mtsr(u8 sr, R rs) { return internal::X_form(31, u8(rs), sr & 0xF, 0, 210, false); }
    consteval u32 mfsr(R rt, u8 sr) { return internal::X_form(31, u8(rt), sr & 0xF, 0, 595, false); }
    consteval u32 mtsrin(R rs, R rb) { return internal::X_form(31, u8(rs), 0, u8(rb), 242, false); }
    consteval u32 mfsrin(R rt, R rb) { return internal::X_form(31, u8(rt), 0, u8(rb), 659, false); }

    // TLB and external-control
    consteval u32 tlbie(R rb) { return internal::X_form(31, 0, 0, u8(rb), 306, false); }
    consteval u32 eciwx(R rt, R ra, R rb) { return internal::X_form(31, u8(rt), u8(ra), u8(rb), 310, false); }
    consteval u32 ecowx(R rs, R ra, R rb) { return internal::X_form(31, u8(rs), u8(ra), u8(rb), 438, false); }

    // Standard extensions
    consteval u32 fres(F fd, F fb, bool rc = false) { return internal::A_form(59, u8(fd), 0, u8(fb), 0, 24, rc); }
    consteval u32 frsqrte(F fd, F fb, bool rc = false) { return internal::A_form(63, u8(fd), 0, u8(fb), 0, 26, rc); }
    consteval u32 fsel(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(63, u8(fd), u8(fa), u8(fb), u8(fc), 23, rc); }
    consteval u32 stfiwx(F fs, R ra, R rb) { return internal::X_form(31, u8(fs), u8(ra), u8(rb), 983, false); }
    consteval u32 tlbsync() { return internal::X_form(31, 0, 0, 0, 566, false); }

    // Paired singles
    consteval u32 psq_l(F ft, R ra, s16 d, bool w, u8 i) { return internal::DW_form(56, u8(ft), u8(ra), d, w, i); }
    consteval u32 psq_lu(F ft, R ra, s16 d, bool w, u8 i) { return internal::DW_form(57, u8(ft), u8(ra), d, w, i); }
    consteval u32 psq_lux(F ft, R ra, R rb, bool w, u8 i) { return internal::XW_form(38, u8(ft), u8(ra), u8(rb), w, i); }
    consteval u32 psq_lx(F ft, R ra, R rb, bool w, u8 i) { return internal::XW_form(6, u8(ft), u8(ra), u8(rb), w, i); }
    consteval u32 psq_st(F fs, R ra, s16 d, bool w, u8 i) { return internal::DW_form(60, u8(fs), u8(ra), d, w, i); }
    consteval u32 psq_stu(F fs, R ra, s16 d, bool w, u8 i) { return internal::DW_form(61, u8(fs), u8(ra), d, w, i); }
    consteval u32 psq_stux(F fs, R ra, R rb, bool w, u8 i) { return internal::XW_form(39, u8(fs), u8(ra), u8(rb), w, i); }
    consteval u32 psq_stx(F fs, R ra, R rb, bool w, u8 i) { return internal::XW_form(7, u8(fs), u8(ra), u8(rb), w, i); }
    consteval u32 ps_mr(F fd, F fb, bool rc = false) { return internal::X_form(4, u8(fd), 0, u8(fb), 72, rc); }
    consteval u32 ps_add(F fd, F fa, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), 0, 21, rc); }
    consteval u32 ps_sub(F fd, F fa, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), 0, 20, rc); }
    consteval u32 ps_mul(F fd, F fa, F fc, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), 0, u8(fc), 25, rc); }
    consteval u32 ps_div(F fd, F fa, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), 0, 18, rc); }
    consteval u32 ps_madd(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 29, rc); }
    consteval u32 ps_msub(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 28, rc); }
    consteval u32 ps_nmadd(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 31, rc); }
    consteval u32 ps_nmsub(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 30, rc); }
    consteval u32 ps_sel(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 23, rc); }
    consteval u32 ps_abs(F fd, F fb, bool rc = false) { return internal::X_form(4, u8(fd), 0, u8(fb), 264, rc); }
    consteval u32 ps_nabs(F fd, F fb, bool rc = false) { return internal::X_form(4, u8(fd), 0, u8(fb), 136, rc); }
    consteval u32 ps_neg(F fd, F fb, bool rc = false) { return internal::X_form(4, u8(fd), 0, u8(fb), 40, rc); }
    consteval u32 ps_res(F fd, F fb, bool rc = false) { return internal::A_form(4, u8(fd), 0, u8(fb), 0, 24, rc); }
    consteval u32 ps_rsqrte(F fd, F fb, bool rc = false) { return internal::A_form(4, u8(fd), 0, u8(fb), 0, 26, rc); }
    consteval u32 ps_muls0(F fd, F fa, F fc, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), 0, u8(fc), 12, rc); }
    consteval u32 ps_muls1(F fd, F fa, F fc, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), 0, u8(fc), 13, rc); }
    consteval u32 ps_madds0(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 14, rc); }
    consteval u32 ps_madds1(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 15, rc); }
    consteval u32 ps_sum0(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 10, rc); }
    consteval u32 ps_sum1(F fd, F fa, F fc, F fb, bool rc = false) { return internal::A_form(4, u8(fd), u8(fa), u8(fb), u8(fc), 11, rc); }
    consteval u32 ps_merge00(F fd, F fa, F fb, bool rc = false) { return internal::X_form(4, u8(fd), u8(fa), u8(fb), 528, rc); }
    consteval u32 ps_merge01(F fd, F fa, F fb, bool rc = false) { return internal::X_form(4, u8(fd), u8(fa), u8(fb), 560, rc); }
    consteval u32 ps_merge10(F fd, F fa, F fb, bool rc = false) { return internal::X_form(4, u8(fd), u8(fa), u8(fb), 592, rc); }
    consteval u32 ps_merge11(F fd, F fa, F fb, bool rc = false) { return internal::X_form(4, u8(fd), u8(fa), u8(fb), 624, rc); }
    consteval u32 ps_cmpo0(CR cr, F fa, F fb) { return internal::X_form(4, internal::cr_field(cr), u8(fa), u8(fb), 32, false); }
    consteval u32 ps_cmpo1(CR cr, F fa, F fb) { return internal::X_form(4, internal::cr_field(cr), u8(fa), u8(fb), 96, false); }
    consteval u32 ps_cmpu0(CR cr, F fa, F fb) { return internal::X_form(4, internal::cr_field(cr), u8(fa), u8(fb), 0, false); }
    consteval u32 ps_cmpu1(CR cr, F fa, F fb) { return internal::X_form(4, internal::cr_field(cr), u8(fa), u8(fb), 64, false); }
    consteval u32 dcbz_l(R ra, R rb) { return internal::X_form(4, 0, u8(ra), u8(rb), 1014, false); }
}

#ifdef TELKIN_REGISTERS
#include <telkin/DefineRegisters.h>
#endif

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
