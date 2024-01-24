/*
 * simple timer interrupt demonstration.
 * Good timer / interrupt discussion at:
     http://www.valvers.com/open-software/raspberry-pi/step04-bare-metal-programming-in-c-pt4
 *
 * Search for "Q:" to find the various questions.   Change the code to answer.
 */
#include "rpi.h"
#include "timer-interrupt.h"

// Q: if you make not volatile?
// A: We will infinite loop (depending on if our compiler settings are fast enough)
static volatile unsigned cnt, period, period_sum;

// client has to define this.
void interrupt_vector(unsigned pc) {
    dev_barrier();
    unsigned pending = GET32(IRQ_basic_pending);

    // if this isn't true, could be a GPU interrupt (as discussed in Broadcom):
    // just return.  [confusing, since we didn't enable!]
    if((pending & RPI_BASIC_ARM_TIMER_IRQ) == 0)
        return;

    // Checkoff: add a check to make sure we have a timer interrupt
    // use p 113,114 of broadcom.
    // A: Check below, we get the Raw IRQ
    //int x = GET32(arm_timer_RAWIRQ);
    //printk("\nTimer Interrupt: %d\n", x);

    /* 
     * Clear the ARM Timer interrupt - it's the only interrupt we have
     * enabled, so we don't have to work out which interrupt source
     * caused us to interrupt 
     *
     * Q: what happens, exactly, if we delete?
     * A: If we comment out/delete, then we just freeze if we don't clear. We keep sending interrupts
     * but we don't clear them out.
     */
    PUT32(arm_timer_IRQClear, 1);

    /*
     * We do not know what the client code was doing: if it was touching a 
     * different device, then the broadcom doc states we need to have a
     * memory barrier.   NOTE: we have largely been living in sin and completely
     * ignoring this requirement for UART.   (And also the GPIO overall.)  
     * This is probably not a good idea and we should be more careful.
     */
    dev_barrier();    
    cnt++;

    // compute time since the last interrupt.
    static unsigned last_clk = 0;
    unsigned clk = timer_get_usec();
    period = last_clk ? clk - last_clk : 0;
    period_sum += period;
    last_clk = clk;

    // Q: what happens (&why) if you uncomment the print statement?
    // A: Difference between time is equal to time between interrupts, but not exactly (a bit more)
    // printk("In interrupt handler at time: %d\n", clk);
}

#include "cycle-count.h"
void notmain() {
    printk("about to install interrupt handlers\n");
    int_init();

    printk("setting up timer interrupts\n");
    // Q: if you change 0x100?
    // A: Longer period between interrupts
    timer_interrupt_init(0x100);

    printk("gonna enable ints globally!\n");
    // Q: what happens (&why) if you don't do?
    // A: We loop infinitely -- need to enable system interrupts, and therefore cnt doesn't increment?
    system_enable_interrupts();
    printk("enabled!\n");

    unsigned start = timer_get_usec();

    // Q: what happens if you enable cache?  Why are some parts
    // the same, some change?
    // enable_cache(); 	
    unsigned iter = 0, sum = 0;
#   define N 20
    while(cnt < N) {
        // Q: if you comment this out?  why do #'s change?
        // A: Maybe you iterate more because printing also takes a non-trivial amount of time,
        // so thus you can't iterate as much?
        printk("iter=%d: cnt = %d, time between interrupts = %d usec (%x)\n", 
                                    iter,cnt, period,period);
        iter++;
    }

    // overly complicated calculation of sec/ms/usec breakdown
    // make it easier to tell the overhead of different changes.
    // not really specific to interrupt handling.
    unsigned tot = timer_get_usec() - start,
             tot_sec    = tot / (1000*1000),
             tot_ms     = (tot / 1000) % 1000,
             tot_usec   = (tot % 1000);
    printk("-----------------------------------------\n");
    printk("summary:\n");
    printk("\t%d: total iterations\n", iter);
    printk("\t%d: tot interrupts\n", N);
    printk("\t%d: iterations / interrupt\n", iter/N);
    printk("\t%d: average period\n\n", period_sum/(N-1));
    printk("total execution time: %dsec.%dms.%dusec\n", 
                    tot_sec, tot_ms, tot_usec);
}
