#include "rpi-asm.h"

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ <0-rfe-example.c>

@ use rfe to load pc and cpsr from the 2 entry
@ array passed in r0.
MK_FN(rfe_asm)
    mov sp, r0
    rfe sp
    asm_not_reached();

@ a trampoline that loads the stack pointer
@ and calls <rfe_c_code>
MK_FN(rfe_trampoline_asm)
    mov sp, #INT_STACK_ADDR
    mov r0, sp
    bl rfe_c_code
    asm_not_reached();

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ <1-rfe-blk.c>

@ tiny change: modify rfe_asm to deal with being 
@ passed a 17-entry register array
MK_FN(blk_rfe_asm)
    mov sp, r0
    add sp, sp, #60
    rfe sp
    asm_not_reached();

@ do not need to modify: same as original, so just
@ call it.
MK_FN(blk_trampoline_asm)
    bl rfe_trampoline_asm
    asm_not_reached();


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@
@ <1-rfe-load-sp.c>

@ copy-paste <blk_rfe_asm> above and change it 
@ so it:
@  1. loads r0-r14 into user mode registers
@     (hint: look back at your <0-user-sp-lr> 
@     code)
@  2. does an rfe 
MK_FN(switchto_user_asm)
    @ Mov r0 to sp first, since will get overwritten by load
    mov sp, r0

    @ Load into user mode registers
    ldm r0, {r0-r14}^

    @ Then you adjust the register
    add sp, sp, #60
    rfe sp
    asm_not_reached();

MK_FN(switchto_any_asm)
    @ Set the stack pointer
    mov sp, r0

    @ Go to the other mode
    ldr r1, [r0, #64]
    msr cpsr, r1
    prefetch_flush(lr)

    @ Load the values
    ldm r0, {r0-r14}

    @ Go back into the old mode
    cps #SUPER_MODE
    prefetch_flush(lr)

    @ Adjust stack pointer
    add sp, sp, #60
    rfe sp
    asm_not_reached();

@ so we don't get linker errors 
.weak rfe_c_code
rfe_c_code:
    asm_not_reached()
