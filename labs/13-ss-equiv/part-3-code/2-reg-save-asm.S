#include "rpi-asm.h"

@ save all the general registers and the spsr in 
@ ascending order onto a block of memory on the
@ stack.  the pointer to the base of this to 
@ do_syscall (defined in `2-reg-save.c` and 
@ `2-reg-save-full.c`).
@
swi_trampoline:
    @ Push values onto the stack
    mov sp, #INT_STACK_ADDR
    sub sp, sp, #68

    @ Get user mode registers
    stm sp, {r0 - r14}^
    add sp, sp, #60

    @ Get pc
    str lr, [sp]
    add sp, sp, #4

    @ Push spsr
    mrs r0, spsr
    str r0, [sp]
    sub sp, sp, #64

    @ Mov stack pointer into r0 + move to function
    mov r0, sp
    bl do_syscall
    @ asm_todo("implement this")

    @ note: this trampoline does not resume.
    asm_not_reached();

@ only handle single step.
.align 5; 
.globl swi_test_handlers
swi_test_handlers:
    b unhandled_reset
    b unhandled_undefined_instruction
    b swi_trampoline
    b unhandled_prefetch_abort 
    b unhandled_data_abort
    b unhandled_interrupt
    b unhandled_fiq


.weak do_syscall
do_syscall:
    asm_not_reached();
