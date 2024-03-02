// complete working example of trivial vm:
//  1. identity map the kernel (code, heap, kernel stack, 
//     exception stack, device memory)
//  2. turn VM on/off making sure we can print.
//  3. with VM on, write to an unmapped address and make sure we 
//     get a fault.
#include "rpi.h"
#include "mmu.h"
#include "pt-vm.h"
#include "libc/bit-support.h"

// from last lab.
#include "switchto.h"
#include "full-except.h"

enum { OneMB = 1024*1024};

// used to store the illegal address we will write.
static volatile uint32_t illegal_addr;

// a trivial fault handler that checks that we got the fault
// we expected.
static void fault_handler(regs_t *r) {
    // Get the pc
    trace("PC: %x\n", r->regs[15]);

    // Read the DFSR
    uint32_t dfsr_val;
    asm volatile("MRC p15, 0, %0, c5, c0, 0" : "=r" (dfsr_val));
    //panic("%d", dfsr_val);
    if (bit_is_off(dfsr_val, 10)) {
        uint32_t reason = bits_get(dfsr_val, 0, 3);
        trace("Reason: %x\n", reason);
    }

    // Give the permissions back (or else we go into an infinite loop)
    int permissions = (DOM_client << 1*2) | (DOM_manager << 3*2);
    staff_domain_access_ctrl_set(permissions);
}

// Fault handler for prefetch
static void prefetch_custom_handler(regs_t *r) {
    // Get the pc
    trace("PC: %x\n", r->regs[15]);

    // Read the DFSR
    uint32_t dfsr_val;
    asm volatile("MRC p15, 0, %0, c5, c0, 0" : "=r" (dfsr_val));
    //panic("%d", dfsr_val);
    if (bit_is_off(dfsr_val, 10)) {
        uint32_t reason = bits_get(dfsr_val, 0, 3);
        trace("Reason: %x\n", reason);
    }

    // Give the permissions back (or else we go into an infinite loop)
    int permissions = (DOM_client << 1*2) | (DOM_manager << 3*2);
    staff_domain_access_ctrl_set(permissions);
}

void notmain(void) { 
    assert(!mmu_is_enabled());

    // ******************************************************
    // 1. one-time initialization.
    //   - create an empty page table (to catch errors).
    //   - initialize all the vm hardware
    //   - compute permissions for no-user-access.


    // map the heap: for lab cksums must be at 0x100000.
    kmalloc_init_set_start((void*)OneMB, OneMB);

    vm_pt_t *pt = vm_pt_alloc(PT_LEVEL1_N);

    // initialize everything, after bootup.
    // <mmu.h>
    vm_mmu_init(dom_no_access);

    // definitions in <pinned-vm.h>
    //
    // see 3-151 for table, or B4-9
    //    given: APX = 0, AP = 1
    //    the bits are: APX << 2 | AP
    uint32_t AXP = 0;
    uint32_t AP = 1;
    uint32_t no_user = AXP << 2 | 1; // no access user (privileged only)
    assert(perm_rw_priv == no_user);

    // armv6 has 16 different domains with their own privileges.
    // just pick one for the kernel.
    enum { 
        dom_kern = 1, // domain id for kernel
        dom_heap = 3, // domain id for heap
    };      

    // ******************************************************
    // 2. setup device memory.
    // 
    // permissions: kernel domain, no user access, 
    // memory rules: strongly ordered, not shared.
    pin_t dev  = pin_mk_global(dom_kern, no_user, MEM_device);
    
    // map all device memory to itself.  ("identity map")
    vm_map_sec(pt, 0x20000000, 0x20000000, dev);
    vm_map_sec(pt, 0x20100000, 0x20100000, dev);
    vm_map_sec(pt, 0x20200000, 0x20200000, dev);

    // ******************************************************
    // 3. setup kernel memory: 

    // protection: same as device.
    // memory rules: uncached access.
    pin_t kern = pin_mk_global(dom_kern, no_user, MEM_uncached);
    pin_t heap_pin = pin_mk_global(dom_heap, no_user, MEM_uncached);

    // Q1: if you uncomment this, what happens / why?
    // kern = pin_mk_global(dom_kern, perm_ro_priv, MEM_uncached);

    // map first two MB for the kernel (1: code, 2: heap).
    //
    // NOTE: obviously it would be better to not have 0 (null) 
    // mapped, but our code starts at 0x8000 and we are using
    // 1mb sections.  you can fix this as an extension.  
    // very useful!
    vm_map_sec(pt, 0, 0, kern);    
    vm_map_sec(pt, OneMB, OneMB, heap_pin);             

    // now map kernel stack (or nothing will work)
    uint32_t kern_stack = STACK_ADDR-OneMB;
    vm_map_sec(pt, kern_stack, kern_stack, kern);   // tlb 5
    uint32_t except_stack = INT_STACK_ADDR-OneMB;

    // Q2: if you comment this out, what happens?
    vm_map_sec(pt, except_stack, except_stack, kern);

    // ******************************************************
    // 4. setup vm hardware.
    //  - page table, asid, pid.
    //  - domain permissions.

    // b4-42: give permissions for all domains.

    // Q3: if you set this to ~0, what happens w.r.t. Q1?
    int permissions = (DOM_client << dom_kern*2) | (DOM_client << dom_heap*2);
    staff_domain_access_ctrl_set(permissions);; 

    // set address space id, page table, and pid.
    // note:
    //  - pid never matters, it's just to help the os.
    //  - asid doesn't matter for this test b/c all entries 
    //    are global
    //  - the page table is empty (since pinning) and is
    //    just to catch errors.
    enum { ASID = 1, PID = 128 };
    mmu_set_ctx(PID, ASID, pt);

    // if you want to see the lockdown entries.
    // lockdown_print_entries("about to turn on first time");

    // ******************************************************
    // 5. turn it on/off, checking that it worked.
    trace("about to enable\n");
    for(int i = 0; i < 10; i++) {
        mmu_enable();

        if(mmu_is_enabled())
            trace("MMU ON: hello from virtual memory!  cnt=%d\n", i);
        else
            panic("MMU is not on?\n");

        mmu_disable();
        assert(!mmu_is_enabled());
        trace("MMU is off!\n");
    }

    // ******************************************************
    // 6. setup exception handling and make sure we get a fault.

    // just like last lab.  setup a data abort handler.
    full_except_install(0);
    full_except_set_data_abort(fault_handler);
    full_except_set_prefetch(prefetch_custom_handler);

    // Disable permissions
    permissions = (DOM_client << dom_kern*2) | (DOM_no_access << dom_heap*2);
    staff_domain_access_ctrl_set(permissions);

    // WE ARE DOING A READ
    illegal_addr = OneMB + 20;
    staff_mmu_enable();
    assert(mmu_is_enabled());
    GET32(illegal_addr);

    // Disable permissions
    permissions = (DOM_client << dom_kern*2) | (DOM_no_access << dom_heap*2);
    staff_domain_access_ctrl_set(permissions);

    // WE ARE DOING A WRITE
    illegal_addr = OneMB + 20;
    assert(mmu_is_enabled());
    PUT32(illegal_addr, 0xdeadbeef);

    // Now we doing the jump baby
    // Write to memory
    PUT32(illegal_addr, 0xe12fff1e);

    // Disable permissions
    permissions = (DOM_client << dom_kern*2) | (DOM_no_access << dom_heap*2);
    staff_domain_access_ctrl_set(permissions);

    BRANCHTO(illegal_addr);
}
