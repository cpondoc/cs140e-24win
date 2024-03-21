// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"
#include "gpio.h"

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    // Set up GPIO pins
    dev_barrier();
    gpio_set_function(15, GPIO_FUNC_ALT5);
    gpio_set_function(14, GPIO_FUNC_ALT5);
    dev_barrier();

    // You need to turn on the UART in AUX. Make sure you read-modify-write --- don't kill the SPIm enables.
    uint32_t AUX_ENB = 0x20215004;
    uint8_t curr_value = GET32(AUX_ENB);
    curr_value = curr_value | 1;
    PUT32(AUX_ENB, curr_value);
    dev_barrier();

    // Immediately disable tx/rx (you don't want to send garbage).
    // Can also be get 8?
    uint32_t CNTL_REG = 0x20215060;
    curr_value = GET32(CNTL_REG);
    curr_value = curr_value & ~0b11;
    PUT32(CNTL_REG, curr_value);

    // Clearing the FIFOs
    uint32_t IIR_REG = 0x20215048;
    PUT32(IIR_REG, 0b110);

    // Disable interrupts
    uint32_t IER_REG = 0x20215044;
    PUT32(IER_REG, 0);

    // Configure baud rate
    uint32_t BAUD_REG = 0x20215068;
    uint32_t baud_rate_reg = 270;
    PUT32(BAUD_REG, baud_rate_reg);

    // Set up in 8-bit mode
    uint32_t LCR_REG = 0x2021504C;
    PUT32(LCR_REG, 0b11);

    // Enable tx/rx
    curr_value = 0b11;
    PUT32(CNTL_REG, curr_value);
    dev_barrier();
}

// disable the uart.
void uart_disable(void) {
    // First, we flush our transactions
    dev_barrier();
    uart_flush_tx();

    // Then, we modify just the singular end bit!
    uint32_t AUX_ENB = 0x20215004;
    uint32_t curr_enabled = GET32(AUX_ENB);
    curr_enabled = curr_enabled & ~0b1;
    PUT32(AUX_ENB, curr_enabled);
    dev_barrier();
}

// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
    // Blocks until it's needed
    dev_barrier();
    uint32_t STAT_REG = 0x20215064;
    while ((GET32(STAT_REG) & 0b1) != 1);

    // Returns one byte from the rx queue
    uint32_t IO_REG = 0x20215040;
    uint8_t return_byte = GET32(IO_REG);
    dev_barrier();
    return return_byte & 0b11111111;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    dev_barrier();

    // We want to get the 5th bit
    uint32_t LSR_REG = 0x20215054;
    uint32_t uart_cp8 = ((GET32(LSR_REG) & 0b100000));

    // Simply check if greater than 0
    dev_barrier();
    return uart_cp8 > 0;
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    // Blocks until TX has space
    dev_barrier();
    while (!uart_can_put8());

    // Put one byte on the tx queue
    uint32_t IO_REG = 0x20215040;
    PUT32(IO_REG, c);
    dev_barrier();
    return 0;
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    dev_barrier();
    // We want to get the 1st bit
    uint32_t LSR_REG = 0x20215054;
    uint32_t has_data = ((GET32(LSR_REG) & 0b1));

    // Simply check if greater than 0
    dev_barrier();
    return has_data > 0;
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    dev_barrier();

    // We want to get the 5th bit
    uint32_t LSR_REG = 0x20215054;
    uint32_t tx_is_empty = ((GET32(LSR_REG) & 0b1000000));

    // Simply check if greater than 0
    dev_barrier();
    return tx_is_empty > 0;
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    dev_barrier();
    while(!uart_tx_is_empty());
    dev_barrier();
}
