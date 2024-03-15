@ Must carefully read B2 of the armv6 manual.  You cannot rely on "well it
@ worked on my test case": mistakes cause stale data, which may or may not
@ show up in your test case (likely not) despite being wildly broken.
@
@ Note: Rd is allegedly a read-only parameter to these instructions, but
@ the manual says SBZ (should be zero).  I think (and the linux source seems
@ to think) that this means we have to explicitly set it to 0.  Not setting
@ Rd=0 is an easy mistake to make.  Perhaps is worth building in (Linux does
@ not, but) --- the cost of an extra mov is negligible given our other 
@ overheads.
@
@ Alot of people put these instructions in inline assembly, wrapped up by 
@ function calls (e.g., cs107e's MMU code).  This practice is dangerous. 
@ For example, if you are flushing caches, you cannot be sure that the 
@ function return, etc does not then reload them, esp w.r.t. the BTB.  I 
@ think the only safe approach is to put directly in assembly so you are 
@ guaranteed no branch, indirect jump, load/store happens in between.
@
@ A lot of MMU code out there appears pretty broken b/c they don't do enough
@ flushing operations and/or rely on gcc code generation to not do the above.

#include "rpi-asm.h"
#include "armv6-coprocessor-asm.h"


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ many page numbers are in 
@    armv6-coprocessor-asm.h
@ and the other armv6-*.h files

// just read the cache type: use C code to figure out if it is unified
// b6-13, b6-14
MK_FN(get_cache_type)
    asm_not_implemented()
    bx lr

// void reset(void)
// clear the dcache, icache, tlbs
MK_FN(mmu_reset)
    // b staff_mmu_reset
    // asm_not_implemented()

    // B2-21 refernced general ordering for cache maintencance operations and
    // B2-22 gives a similar example for how to properly invalidate
    mov r0, #0
    INV_DCACHE(r0)
    INV_TLB(r0)
    INV_ICACHE(r0)  
    DSB(r0)
    PREFETCH_FLUSH(r0)
    bx lr

// void cp15_domain_ctrl_wr(uint32_t dom_reg);
//  need to do cp15 sync: flush_btb, dsb, prefetch flush
MK_FN(cp15_domain_ctrl_wr)
    // b staff_cp15_domain_ctrl_wr

    // asm_not_implemented()
    mov r1, #0
    DOMAIN_CTRL_WR(r0) // this is from arm1176 3-63 as stated in armv6-coprocessor-asm.h
    PREFETCH_FLUSH(r1) // on page B2-24, it says that we need to prefetch flush if we write to cp15
    bx lr

MK_FN(cp15_domain_ctrl_rd)
    // b staff_cp15_domain_ctrl_wr

    // asm_not_implemented()
    DOMAIN_CTRL_RD(r0) // this is from arm1176 3-63 as stated in armv6-coprocessor-asm.h
    bx lr

// void cp15_set_procid_ttbr0(uint32_t proc_and_asid, fld_t *pt);
// sequence from b2-25
MK_FN(cp15_set_procid_ttbr0)
    // b staff_cp15_set_procid_ttbr0
    // asm_not_implemented()
    mov r2, #0 // B2-25
    DSB(r2) // 3-129
    ASID_SET(r2) // B2-25
    PREFETCH_FLUSH(r2) // B2-25
    TTBR0_SET(r1) // B2-25
    mov r2, #0 // B2-25
    TTBR1_SET(r2) // B2-25
    PREFETCH_FLUSH(r2) //B2-24
    FLUSH_BTB(r2) //B2-24
    PREFETCH_FLUSH(r2) //B2-24
    ASID_SET(r0) // B2-25
    PREFETCH_FLUSH(r2) // B2-25    // DO WE NEED TO DO THIS?
    bx lr

// void mmu_disable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_disable_set_asm)
    // b staff_mmu_disable_set_asm

    @ note: this piece of code is mapped into the sample place in both
    @ MMU-enabled and MMU-disabled so BTB and Prefetch entries work identically
    @ to both MMU-enable and disable.   Without this requirement I believe is
    @ very hard to get right (perhaps impossible?) since we cannot flush
    @ the BTB and prefetch and switch MMU on/off atomically.  thus, would
    @ flush, but instructions could get fetched before we could disable,
    @ defeating the purpose.
    mov r1, #0
    CLEAN_INV_DCACHE(r1) // page 6-9 and from lab
    DSB(r1)
    CONTROL_REG1_WR(r0) // 6-9 (choose this register because given cp15_ctrl_reg1_t)
    PREFETCH_FLUSH(r1)
    INV_ICACHE(r1) // lab writeup said to do this
    FLUSH_BTB(r1) // B2-24
    DSB(r1) // B2-21
    PREFETCH_FLUSH(r1)
    // asm_not_implemented()
    bx lr

// void mmu_enable_set_asm(cp15_ctrl_reg1_t c);
MK_FN(mmu_enable_set_asm)
    // b staff_mmu_enable_set_asm
    // asm_not_implemented()
    mov r1, #0
    CONTROL_REG1_RD(r2) // disable the icache
    BIC r2, #0b1000000000000
    CONTROL_REG1_WR(r2)
    PREFETCH_FLUSH(r1)
    INV_ICACHE(r1) // 6-9
    DSB(r1) // B2-21 and B2-22
    CONTROL_REG1_WR(r0) // 6-9 (choose this register because given cp15_ctrl_reg1_t)
    PREFETCH_FLUSH(r1)
    FLUSH_BTB(r1) // B2-24
    PREFETCH_FLUSH(r1) // B2-24
    bx lr

MK_FN(mmu_sync_pte_mods)
    // b staff_mmu_sync_pte_mods

    // asm_not_implemented()
    // all from following B2-23
    mov r0, #0
    CLEAN_INV_DCACHE(r0)
    INV_ICACHE(r0)
    DSB(r0)
    INV_TLB(r0)
    FLUSH_BTB(r0)
    DSB(r0)
    PREFETCH_FLUSH(r0)
    bx lr

MK_FN(cp15_ctrl_reg1_rd)
    CONTROL_REG1_RD(r0) // b4-40 as noted in armv6-coprocessor-asm.h
    bx lr

MK_FN(cp15_ctrl_reg1_wr)
    CONTROL_REG1_WR(r0) // b4-40 as noted in armv6-coprocessor-asm.h
    bx lr