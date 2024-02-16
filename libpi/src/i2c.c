/*
 * simplified i2c implementation --- no dma, no interrupts.  the latter
 * probably should get added.  the pi's we use can only access i2c1
 * so we hardwire everything in.
 *
 * datasheet starts at p28 in the broadcom pdf.
 *
 */
#include "rpi.h"
#include "libc/helper-macros.h"
#include "i2c.h"
#include "gpio.h"
#include "libc/bit-support.h"

typedef struct {
	uint32_t control; // "C" register, p29
	uint32_t status;  // "S" register, p31

#	define check_dlen(x) assert(((x) >> 16) == 0)
	uint32_t dlen; 	// p32. number of bytes to xmit, recv
					// reading from dlen when TA=1
					// or DONE=1 returns bytes still
					// to recv/xmit.  
					// reading when TA=0 and DONE=0
					// returns the last DLEN written.
					// can be left over multiple pkts.

    // Today address's should be 7 bits.
#	define check_dev_addr(x) assert(((x) >> 7) == 0)
	uint32_t 	dev_addr;   // "A" register, p 33, device addr 

	uint32_t fifo;  // p33: only use the lower 8 bits.
#	define check_clock_div(x) assert(((x) >> 16) == 0)
	uint32_t clock_div;     // p34
	// we aren't going to use this: fun to mess w/ tho.
	uint32_t clock_delay;   // p34
	uint32_t clock_stretch_timeout;     // broken on pi.
} RPI_i2c_t;

// offsets from table "i2c address map" p 28
_Static_assert(offsetof(RPI_i2c_t, control) == 0, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, status) == 0x4, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, dlen) == 0x8, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, dev_addr) == 0xc, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, fifo) == 0x10, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, clock_div) == 0x14, "wrong offset");
_Static_assert(offsetof(RPI_i2c_t, clock_delay) == 0x18, "wrong offset");

/*
 * There are three BSC masters inside BCM. The register addresses starts from
 *	 BSC0: 0x7E20_5000 (0x20205000)
 *	 BSC1: 0x7E80_4000
 *	 BSC2 : 0x7E80_5000 (0x20805000)
 * the PI can only use BSC1.
 */
static volatile RPI_i2c_t *i2c = (void*)0x20804000; 	// BSC1

// extend so this can fail.
int i2c_write(unsigned addr, uint8_t data[], unsigned nbytes) {
	// Before starting, wait until transfer is not active
	dev_barrier();
	while ((get32(&(i2c->status)) & 0b1) == 1);

    // Set status to done
	int status = get32(&(i2c->status));
	status = bit_set(status, 1);
	put32(&(i2c->status), status);

	// Then: check in status that: fifo is empty, there was no clock stretch timeout 
	// and there were no errors. We shouldn't see any of these today.
	status = get32(&(i2c->status));
	if ((status & (1101 << 6)) == 0) {
		panic("There were errors!");
	}

	// Set the device address and length.
	put32(&(i2c->dev_addr), addr);
	put32(&(i2c->dlen), nbytes);

	// Set the control reg to write and start transfer.
	int control = get32(&(i2c->control));
	control = bit_clr(control, 0);
	control = bit_set(control, 7);
	put32(&(i2c->control), control);

	// Wait until we can send a byte, then send the byte
	for (int i = 0; i < nbytes; i++) {
		while ((get32(&(i2c->status)) & 0b10000) == 0);
		put32(&(i2c->fifo), data[i]);
	}

	// Wait until the transfer is done and assert no active transfer
	while ((get32(&(i2c->status)) & 0b10) == 0);
	status = get32(&(i2c->status));
	assert((status & 0b1) == 0);

	// Sanity check no ACK error or clock timeout
	assert((status & 1 << 8) == 0);
	assert((status & 1 << 9) == 0);

	// Finished
	dev_barrier();
	return 1;
}

// extend so it returns failure.
int i2c_read(unsigned addr, uint8_t data[], unsigned nbytes) {
	// Before starting, wait until transfer is not active
	dev_barrier();
	while ((get32(&(i2c->status)) & 0b1) == 1);

	// Set status to done
	int status = get32(&(i2c->status));
	status = bit_set(status, 1);
	put32(&(i2c->status), status);
	
	// Then: check in status that: fifo is empty, there was no clock stretch timeout 
	// and there were no errors. We shouldn't see any of these today.
	status = get32(&(i2c->status));
	if ((status & (1101 << 6)) == 0) {
		panic("There were errors!");
	}

	// Set the device address and length.
	put32(&(i2c->dev_addr), addr);
	put32(&(i2c->dlen), nbytes);

	// Set the control reg to read and start transfer.
	int control = get32(&(i2c->control));
	control = bit_set(control, 0);
	control = bit_set(control, 7);
	put32(&(i2c->control), control);

	// Wait until we can read a byte, then read the byte
	// while ((get32(&(i2c->status)) & 0b10) == 0);
	for (int i = 0; i < nbytes; i++) {
		while ((get32(&(i2c->status)) & 0b100000) == 0);
		data[i] = get32(&(i2c->fifo));
	}

	// Wait until the transfer is done and assert no active transfer
	while ((get32(&(i2c->status)) & 0b10) == 0);
	status = get32(&(i2c->status));
	assert((status & 0b1) == 0);

	// Sanity check no ACK error or clock timeout
	assert((status & 1 << 8) == 0);
	assert((status & 1 << 9) == 0);
	dev_barrier();
	return 1;
}

void i2c_init(void) {
    // Set up GPIO pin 2 and pin 3
	dev_barrier();
    gpio_set_function(2, GPIO_FUNC_ALT0);
    gpio_set_function(3, GPIO_FUNC_ALT0);
    dev_barrier();

	// Set up C Register (Page 31)
	dev_barrier();
	put32(&(i2c->control), (1 << 15) + (1 << 5));

	// Set up the clock divider (Page 34)
	put32(&(i2c->clock_div), 0);

	// Clear the S register (Page 31)
	put32(&(i2c->status), 0b1100000010);

	// Sanity check that there is no active transfer
	int status = get32(&(i2c->status));
	assert((status & 0b1) == 0);

	// Sanity check no ACK error or clock timeout
	assert((status & 1 << 8) == 0);
	assert((status & 1 << 9) == 0);
	dev_barrier();
}

// shortest will be 130 for i2c accel.
void i2c_init_clk_div(unsigned clk_div) {
    todo("same as init but set the clock divider");
}
