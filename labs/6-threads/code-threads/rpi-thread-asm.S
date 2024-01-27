/*
 * implement these.  ordered in difficulty.
 */
#include "rpi-asm.h"

@ return the current stack pointer.
MK_FN(rpi_get_sp)
    asm_todo("implement this")
    bx lr

MK_FN(rpi_cswitch)
    @ save the current register values into <old_save_area>
    push {r4-r11, lr}
    str sp, [r0]

    @ load the values in <new_save_area> into the registers
    mov sp, r1
    pop {r4-r11, lr}

    @ Return back and restore stack pointer
    bx lr

@ [Make sure you can answer: why do we need to do this?]
@
@ use this to setup each thread for the first time.
@ setup the stack so that when cswitch runs it will:
@	- <code> into r1, 
@	- <arg> into r0
@ 
MK_FN(rpi_init_trampoline)
    @ load in the parameters (arg, then code)
    mov r0, r4
    mov r1, r5

    @ go to code
    blx r1

    @ branch and link 
    bl rpi_exit
