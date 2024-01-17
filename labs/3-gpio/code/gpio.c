/*
 * Implement the following routines to set GPIO pins to input or output,
 * and to read (input) and write (output) them.
 *
 * DO NOT USE loads and stores directly: only use GET32 and PUT32
 * to read and write memory.  Use the minimal number of such calls.
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"

// see broadcomm documents for magic addresses.
enum {
    GPIO_BASE = 0x20200000,
    gpio_fsel0 = GPIO_BASE, // Add a constant here to make it clear
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34)
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_out put
//

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
  /*
  Steps:
  1. Figure out proper register to write to
  2. Get 32 and clear
  3. Put 32 again, with the appropriate bits
  */
  // Return
  if (pin >= 32) {
    return;
  }

  // Find out proper register
  unsigned reg_num = pin / 10;
  unsigned reg_addr = gpio_fsel0;
  while (reg_num > 0) {
      reg_addr = reg_addr + 0x00000004;
      reg_num -= 1;
  }

  // Get 32, and clear out corresponding bits
  uint32_t pins = GET32(reg_addr);
  uint32_t shift = (pin % 10) * 3;
  uint32_t mask = 0x00000007 << shift;
  uint32_t value = 0x00000001 << shift;

  // PUT32 again with the appropriate bits
  pins &= ~mask;
  pins |= value;
  PUT32(reg_addr, pins);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
  if(pin >= 32) {
    return;
  }
  
  // Get the pins and then just set the value
  PUT32(gpio_set0, (1 << pin));

  // implement this
  // use <gpio_set0>
}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
  if (pin >= 32) {
    return;
  }

  // Put to the clear address
  PUT32(gpio_clr0, (1 << pin));

  // implement this
  // use <gpio_clr0>
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
  // Return
  if (pin >= 32) {
    return;
  }

  // Find out proper register
  unsigned reg_num = pin / 10;
  unsigned reg_addr = gpio_fsel0;
  while (reg_num > 0) {
      reg_addr = reg_addr + 0x00000004;
      reg_num -= 1;
  }

  // Get 32, and clear out corresponding bits
  uint32_t pins = GET32(reg_addr);
  uint32_t shift = (pin % 10) * 3;
  uint32_t mask = 0x00000007 << shift;

  // PUT32 again with the appropriate bits
  pins &= ~mask;
  PUT32(reg_addr, pins);
}

// return the value of <pin>
int gpio_read(unsigned pin) {
  // Check this value
  uint32_t pins = GET32(gpio_lev0);
  uint32_t mask = 1 << pin;
  pins &= mask;
  if (pins > 0) {
    return 1;
  }
  return 0;
}
