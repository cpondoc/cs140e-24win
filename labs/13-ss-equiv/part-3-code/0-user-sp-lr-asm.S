#include "rpi-asm.h"
    
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ get USER banked regs by switching to SYSTEM mode and back.
@ if we switched to USER, we would need to use a system call to 
@ get back.

@ get the USER sp by switching to SYS_MODE 
@ and back.
MK_FN(user_sp_get)
    cps #SYS_MODE
    prefetch_flush(r1);

    @ USER And SYSTEM share the same sp/lr/cpsr
    mov r0, sp

    cps #SUPER_MODE
    prefetch_flush(r1);

    bx lr

@ set the USER sp
MK_FN(user_sp_set)
    cps #SYS_MODE
    prefetch_flush(r1);

    @ USER And SYSTEM share the same sp/lr/cpsr
    mov sp, r0

    cps #SUPER_MODE
    prefetch_flush(r1);

    bx lr

@ get the USER lr 
MK_FN(user_lr_get)
    cps #SYS_MODE
    prefetch_flush(r1);

    @ USER And SYSTEM share the same sp/lr/cpsr
    mov r0, lr

    cps #SUPER_MODE
    prefetch_flush(r1);

    bx lr

@ set USER lr 
MK_FN(user_lr_set)
    cps #SYS_MODE
    prefetch_flush(r1);

    @ USER And SYSTEM share the same sp/lr/cpsr
    mov lr, r0

    cps #SUPER_MODE
    prefetch_flush(r1);

    bx lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ get USER banked regs by using ldm/stm instructions with the ^
@ modifier.

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_sp_get)
    stm r0, {sp}^
    bx lr

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_sp_set)
    ldm r0, {sp}^
    bx lr

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_lr_get)
    stm r0, {lr}^
    bx lr

@ address to a uint32_t word is passed in r0
MK_FN(mem_user_lr_set)
    ldm r0, {lr}^
    bx lr

@ address to an array with two uint32_t words 
@ is passed in r0
MK_FN(mem_user_sp_lr_get)
    stm r0, {sp, lr}^
    bx lr

@ address to an array with two uint32_t words 
@ is passed in r0
MK_FN(mem_user_sp_lr_set)
    ldm r0, {sp, lr}^
    bx lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@ switch to a given mode to get the values we want.
@
@ can test by using the code above to test and then switching

@ void mode_get_lr_sp_asm(mode, &sp, &lr)
MK_FN(mode_get_lr_sp_asm)
    msr cpsr_c, r0
    prefetch_flush(r3);

    @ USER And SYSTEM share the same sp/lr/cpsr
    str sp, [r1]
    str lr, [r2]

    @ Can we just go back to supervisor mode?
    cps #SUPER_MODE
    prefetch_flush(r3);
    bx lr

