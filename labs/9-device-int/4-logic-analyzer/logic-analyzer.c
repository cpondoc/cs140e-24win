/*
 * engler, cs140e: simple tests to check that you are handling rising and falling
 * edge interrupts correctly.
 *
 * NOTE: make sure you connect your GPIO 20 to your GPIO 21 (i.e., have it "loopback")
 */
#include "rpi.h"
#include "timer-interrupt.h"
#include "libc/circular.h"
#include "sw-uart.h"
#include "cycle-count.h"

static cq_t uartQ;

enum { out_pin = 21, in_pin = 20 };
static volatile unsigned n_rising_edge, n_falling_edge;
static unsigned global_counter = 0;


// client has to define this.
void interrupt_vector(unsigned pc) {
    // 1. compute the cycles since the last event and push32 in the queue
    // 2. push the previous pin value in the circular queue (so if was
    //    falling edge: previous must have been a 1).
    //
    // notes:
    //  - make sure you clear the GPIO event!
    //  - using the circular buffer is pretty slow. should tune this.
    //    easy way is to use a uint32_t array where the counter is volatile.
    
    // Calculate the number of cycles
    unsigned s = cycle_cnt_read();
    unsigned num_cycles = s - global_counter;
    global_counter = s;
    cq_push32(&uartQ, num_cycles);

    // Check for interrupts
    dev_barrier();
    if (gpio_read(in_pin) == 0) {
        cq_push32(&uartQ, 1);
        n_falling_edge += 1;
    } else {
        cq_push32(&uartQ, 0);
        n_rising_edge += 1;
    }

    // Clearing the GPIO event
    gpio_event_clear(in_pin);
    dev_barrier();
}

void notmain() {
    cq_init(&uartQ,1);
    int_init();
    caches_enable();

    // use pin 20 for tx, 21 for rx
    sw_uart_t u = sw_uart_init(out_pin,in_pin, 115200);
    gpio_int_rising_edge(in_pin);
    gpio_int_falling_edge(in_pin);

    uint32_t cpsr = cpsr_int_enable();

    assert(gpio_read(in_pin) == 1);

    // assert(gpio_read(in_pin) == 1);

    // starter code.
    // make sure this works first, then try to measure the overheads.
    delay_ms(100);

    // this will cause transitions every time, so you can compare times.
    for(int l = 0; l < 2; l++) {
        unsigned b = 0b01010101;
        sw_uart_put8(&u, b);
        delay_ms(100);
        printk("nevent=%d\n", cq_nelem(&uartQ)/8);

        while(!cq_empty(&uartQ)) {
            unsigned ncycles    = cq_pop32(&uartQ);
            unsigned v          = cq_pop32(&uartQ);
            printk("\tv=%d, cyc=%d\n", v, ncycles);
        }
    }
    trace("SUCCESS!\n");
}
