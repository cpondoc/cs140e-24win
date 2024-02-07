// engler, cs140 put your gpio-int implementations in here.
#include "rpi.h"

// Enums for the address
enum {
    BASE = 0x20000000,
    IRQ_TWO_PENDING = BASE + 0xB208,
    RISING_EDGE = BASE + 0x20004C,
    FALLING_EDGE = BASE + 0x200058,
    ENABLE_IRQS = BASE + 0xB214,
    EVENT_DETECTED = BASE + 0x200040,
};

// returns 1 if there is currently a GPIO_INT0 interrupt, 
// 0 otherwise.
//
// note: we can only get interrupts for <GPIO_INT0> since the
// (the other pins are inaccessible for external devices).
int gpio_has_interrupt(void) {
    // Page 117 -- if enabled, we know the interrupt can be pending
    uint32_t value = GET32(IRQ_TWO_PENDING);

    // We want to get bit 17
    value = value >> 17;
    return DEV_VAL32(value & 0b1);
}

// p97 set to detect rising edge (0->1) on <pin>.
// as the broadcom doc states, it  detects by sampling based on the clock.
// it looks for "011" (low, hi, hi) to suppress noise.  i.e., its triggered only
// *after* a 1 reading has been sampled twice, so there will be delay.
// if you want lower latency, you should us async rising edge (p99)
void gpio_int_rising_edge(unsigned pin) {
    // Check for correct pin
    if (pin >= 32) {
        panic("Incorrect -- pin is not accessible!");
    }

    // Read in detected event
    dev_barrier();
    uint32_t value = GET32(RISING_EDGE);
    uint32_t mask = 1 << pin;

    // Set the pin to 1 to clear, and then put back into the register value
    value = value | mask;
    PUT32(RISING_EDGE, value);
    dev_barrier();

    // Enable interrupts
    value = 1 << 17;
    PUT32(ENABLE_IRQS, value);
    dev_barrier();
}

// p98: detect falling edge (1->0).  sampled using the system clock.  
// similarly to rising edge detection, it suppresses noise by looking for
// "100" --- i.e., is triggered after two readings of "0" and so the 
// interrupt is delayed two clock cycles.   if you want  lower latency,
// you should use async falling edge. (p99)
void gpio_int_falling_edge(unsigned pin) {
    // Check for correct pin
    if (pin >= 32) {
        panic("Incorrect -- pin is not accessible!");
    }

    // Read in detected event
    dev_barrier();
    uint32_t value = GET32(FALLING_EDGE);
    uint32_t mask = 1 << pin;

    // Set the pin to 1 to clear, and then put back into the register value
    value = value | mask;
    PUT32(FALLING_EDGE, value);
    dev_barrier();

    // Enable interrupts
    value = 1 << 17;
    PUT32(ENABLE_IRQS, value);
    dev_barrier();
}

// p96: a 1<<pin is set in EVENT_DETECT if <pin> triggered an interrupt.
// if you configure multiple events to lead to interrupts, you will have to 
// read the pin to determine which caused it.
int gpio_event_detected(unsigned pin) {
    // Check for correct pin
    if (pin >= 32) {
        panic("Incorrect -- pin is not accessible!");
    }
    
    // Detect an event
    dev_barrier();
    uint32_t value = GET32(EVENT_DETECTED);
    uint32_t mask = 1 << pin;

    // Return if the bit at the pin offset is greater than 0
    dev_barrier();
    return DEV_VAL32((value & mask) > 0);
}

// p96: have to write a 1 to the pin to clear the event.
void gpio_event_clear(unsigned pin) {
    // Check for correct pin
    if (pin >= 32) {
        return;
    }
    
    // Read in detected event
    dev_barrier();
    uint32_t value = 1 << pin;

    // Set the pin to 1 to clear, and then put back into the register value
    PUT32(EVENT_DETECTED, value);
    dev_barrier();
}
