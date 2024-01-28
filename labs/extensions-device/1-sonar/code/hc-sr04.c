#include "rpi.h"
#include "hc-sr04.h"

// gpio_read(pin) until either:
//  1. gpio_read(pin) != v ==> return 1.
//  2. <timeout> microseconds have passed ==> return 0
static int read_while_eq(int pin, int v, unsigned timeout) {
    // Set up the current time and run while
    uint32_t s = timer_get_usec();
    while (1) {
        // Check pin reading
        int reading = gpio_read(pin);
        if (reading != v) {
            return 1;
        }
        
        // Check timeout
        uint32_t e = timer_get_usec();
        if ((e - s) >= timeout) {
            return 0;
        }
    }
}

// initialize:
//  1. setup the <trigger> and <echo> GPIO pins.
// 	2. init the HC-SR04 (pay attention to time delays here)
// 
// Pay attention to the voltages on:
//    - Vcc
//    - Vout.
//
// Troubleshooting:
// 	1. there are conflicting accounts of what value voltage you
//	need for Vcc.
//	
// 	2. the initial 3 page data sheet you'll find sucks; look for
// 	a longer one. 
//
// The comments on the sparkfun product page might be helpful.
hc_sr04_t hc_sr04_init(unsigned trigger, unsigned echo) {
    hc_sr04_t h = { .trigger = trigger, .echo = echo };

    // Set input and output
    gpio_set_output(trigger);
    gpio_set_input(echo);
    gpio_set_off(trigger);

    // Keep setting it up for 10 microseconds? Should we do it?
    /* gpio_set_on(trigger);
    delay_us(10);
    gpio_set_off(h.trigger); */

    // Checking for staff
    //return staff_hc_sr04_init(trigger, echo);

    return h;
}

// get distance.
//	1. do a send (again, pay attention to any needed time 
// 	delays)
//
//	2. measure how long it takes and compute round trip
//	by converting that time to distance using the datasheet
// 	formula
//
// troubleshooting:
//  0. We don't have floating point or integer division.
//
//  1. The pulse can get lost!  Make sure you use the timeout read
//  routine you write.
// 
//	2. readings can be noisy --- you may need to require multiple
//	high (or low) readings before you decide to trust the 
// 	signal.
//
int hc_sr04_get_distance(hc_sr04_t h, unsigned timeout_usec) {
    // Make a new send
    gpio_set_on(h.trigger);
    delay_us(10);
    gpio_set_off(h.trigger);
    
    // Wait until we finally start getting the value
    while (gpio_read(h.echo) != 1);

    // Check when pin is no longer equal to 1, and then return
    uint32_t s = timer_get_usec();
    int result = read_while_eq(h.echo, 1, timeout_usec);
    uint32_t e = timer_get_usec();

    // Return the appropriate amount of time
    if (result == 0) {
        return timeout_usec / 148;
    }
    return (e - s) / 148;
}
