#include "rpi.h"
#include "mbox.h"

uint32_t rpi_temp_get(void) ;

#include "cycle-count.h"

// To update over time
static volatile int dummy_var = 1;

// compute cycles per second using
//  - cycle_cnt_read();
//  - timer_get_usec();
unsigned cyc_per_sec(void) {
    // Get start cycle and timer
    uint32_t s = cycle_cnt_read();
    uint32_t s_time = timer_get_usec();

    // Wait the number of microseconds in a second
    while (timer_get_usec() - s_time < 1000000);

    // Read the current cycle count and return difference
    uint32_t e = cycle_cnt_read();
    return e - s;
}

unsigned get_fahrenheit_temp(unsigned x) {
    unsigned C = x / 1000;
    unsigned F = (C * 9 / 5) + 32;
    return F;
}

// Seems like shitting itself around clock rate of: 1100000000 Hz.
void notmain(void) { 
    // Header message
    output("Experimenting with overclocking the pi!\n\n");

    // Get initial clock rate from function and mailbox
    output("Initial Cycles Per Second: %u\n", cyc_per_sec());
    uint32_t starting_temp = get_fahrenheit_temp(rpi_temp_get());
    output("Initial Temperature: %u\n", starting_temp);
    output("Mailbox Clock Rate: %u\n\n", get_clock_rate());

    // Variables for initial and max clock rate, and timer delay
    uint32_t clock_rate = 750000000;
    uint32_t max_clock_rate = 1050000001;
    uint32_t timer_delay = 600000;

    // Continuously update the clock rate
    while (clock_rate < max_clock_rate) {
        output("Now, set clock rate to: %d Hz\n", clock_rate);
        set_clock_rate(clock_rate);

        // Delay for a period of time and do computations
        uint32_t start_time = timer_get_usec();
        while (timer_get_usec() - start_time < timer_delay) {
            dummy_var *= 2;
        }

        // Get the new clock rate
        output("Updated Cycles Per Second: %u\n", cyc_per_sec());
        output("Updated Temperature: %u\n", get_fahrenheit_temp(rpi_temp_get()));
        output("Updated Mailbox Clock Rate: %u\n\n", get_clock_rate());

        // Update clock rate
        clock_rate += 50000000;
    }
}