// very dumb, simple interface to wrap up watchpoints better.
// only handles a single watchpoint.
//
// You should be able to take most of the code from the 
// <1-watchpt-test.c> test case where you already did 
// all the needed operations.  This interface just packages 
// them up a bit.
//
// possible extensions:
//   - do as many as hardware supports, perhaps a handler for 
//     each one.
//   - make fast.
//   - support multiple independent watchpoints so you'd return
//     some kind of structure and pass it in as a parameter to
//     the routines.
#include "rpi.h"
#include "vector-base.h"
#include "full-except.h"
#include "armv6-debug-impl.h"
#include "mini-watch.h"
#include <math.h>



// we have a single handler: so just use globals.
static watch_handler_t watchpt_handler = 0;
static void *watchpt_data = 0;

// is it a load fault?
static int mini_watch_load_fault(void) {
    return datafault_from_ld();
}

// if we have a dataabort fault, call the watchpoint
// handler.
static void watchpt_fault(regs_t *r) {
    // watchpt handler.
    if(was_brkpt_fault())
        panic("should only get debug faults!\n");
    if(!was_watchpt_fault())
        panic("should only get watchpoint faults!\n");
    if(!watchpt_handler)
        panic("watchpoint fault without a fault handler\n");

    watch_fault_t w = {0};

    // Set up the watch fault structure
    w.fault_pc = watchpt_fault_pc();
    w.is_load_p = mini_watch_load_fault();
    w.regs = r;

    // Getting the appropriate address
    uint32_t addr = cp14_wvr0_get();
    uint32_t offset = bits_get(cp14_wcr0_get(), 5, 8);
    if (offset == 2) {
        addr += 1;
    } else if (offset == 4) {
        addr += 2;
    } else if (offset == 8) {
        addr += 3;
    }
    w.fault_addr = (void *) addr;

    // Call watchpoint handler
    watchpt_handler(watchpt_data, &w);  

    // in case they change the regs.
    switchto(w.regs);
}

// setup:
//   - exception handlers, 
//   - cp14, 
//   - setup the watchpoint handler
// (see: <1-watchpt-test.c>
void mini_watch_init(watch_handler_t h, void *data) {
    // Set up exception handlers
    full_except_install(0);
    full_except_set_data_abort(watchpt_fault);

    // cp14 enable
    cp14_enable();

    // just started, should not be enabled.
    assert(!cp14_bcr0_is_enabled());
    assert(!cp14_bcr0_is_enabled());

    // Set up the watchpoint handler and data
    watchpt_handler = h;
    watchpt_data = data;
}

// set a watch-point on <addr>.
// wvr set, then also set the appropriate bits in wcr
void mini_watch_addr(void *addr) {
    // Set the address
    cp14_wvr0_set((uint32_t) addr);

    // Also set the appropriate bits
    uint32_t b = 0b000000000000000011111;
    uint32_t offset = ((uint32_t) addr) & 0b11;
    b = bits_set(b, 5, 8, 1 << offset);
    cp14_wcr0_set(b);
    
    // Assertion test
    assert(cp14_wcr0_is_enabled());
}

// disable current watchpoint <addr>
// note -- wcr disable, don't need the address?
void mini_watch_disable(void *addr) {
    cp14_wcr0_disable();
}

// return 1 if enabled.
// cp14_is_enabled
int mini_watch_enabled(void) {
    return cp14_wcr0_is_enabled();
}

// called from exception handler: if the current 
// fault is a watchpoint, return 1
// was_watchpt_fault
int mini_watch_is_fault(void) { 
    return was_watchpt_fault();
}
