// engler, cs140e: starter code for trivial threads package.
#include "rpi.h"
#include "rpi-thread.h"
#include "redzone.h"

// tracing code.  set <trace_p>=0 to stop tracing
enum { trace_p = 1};
#define th_trace(args...) do {                          \
    if(trace_p) {                                       \
        trace(args);                                   \
    }                                                   \
} while(0)

/***********************************************************************
 * datastructures used by the thread code.
 *
 * you don't have to modify this.
 */

#define E rpi_thread_t
#include "libc/Q.h"

// currently only have a single run queue and a free queue.
// the run queue is FIFO.
static Q_t runq, freeq;
static rpi_thread_t *cur_thread;        // current running thread.
static rpi_thread_t *scheduler_thread;  // first scheduler thread.

// monotonically increasing thread id: won't wrap before reboot :)
static unsigned tid = 1;

/***********************************************************************
 * simplistic pool of thread blocks: used to make alloc/free faster (plus,
 * our kmalloc doesn't have free (other than reboot).
 *
 * you don't have to modify this.
 */

// total number of thread blocks we have allocated.
static unsigned nalloced = 0;

// keep a cache of freed thread blocks.  call kmalloc if run out.
static rpi_thread_t *th_alloc(void) {
    redzone_check(0);
    rpi_thread_t *t = Q_pop(&freeq);

    if(!t) {
        t = kmalloc_aligned(sizeof *t, 8);
        nalloced++;
    }
#   define is_aligned(_p,_n) (((unsigned)(_p))%(_n) == 0)
    demand(is_aligned(&t->stack[0],8), stack must be 8-byte aligned!);
    t->tid = tid++;
    return t;
}

static void th_free(rpi_thread_t *th) {
    redzone_check(0);
    // push on the front in case helps with caching.
    Q_push(&freeq, th);
}


/***********************************************************************
 * implement the code below.
 */

// stack offsets we expect.
enum {
    R4_OFFSET = 0,
    R5_OFFSET,
    R6_OFFSET,
    R7_OFFSET,
    R8_OFFSET,
    R9_OFFSET,
    R10_OFFSET,
    R11_OFFSET,
    R14_OFFSET = 8,
    LR_OFFSET = 8
};

// return pointer to the current thread.  
rpi_thread_t *rpi_cur_thread(void) {
    return cur_thread;
}

// create a new thread.
rpi_thread_t *rpi_fork(void (*code)(void *arg), void *arg) {
    redzone_check(0);
    rpi_thread_t *t = th_alloc();

    // write this so that it calls code,arg.
    void rpi_init_trampoline(void);

    /*
     * must do the "brain surgery" (k.thompson) to set up the stack
     * so that when we context switch into it, the code will be
     * able to call code(arg).
     *
     *  1. write the stack pointer with the right value.
     *  2. store arg and code into two of the saved registers.
     *  3. store the address of rpi_init_trampoline into the lr
     *     position so context switching will jump there.
     */
    // Put stack at the top
    t->saved_sp = &t->stack[0];

    // Store arg and code to saved registers
    t->saved_sp[(THREAD_MAXSTACK - 1) - (8 - R4_OFFSET)] = (uint32_t)arg;
    t->saved_sp[(THREAD_MAXSTACK - 1) - (8 - R5_OFFSET)] = (uint32_t)code;

    // Store the address of trampoline into the LR position
    t->saved_sp[(THREAD_MAXSTACK - 1) - (8 - LR_OFFSET)] = (uint32_t)&rpi_init_trampoline;

    // Update the saved stack pointer to the end of registers (stack grows downward)
    t->saved_sp = &t->stack[THREAD_MAXSTACK - 1 - (8 - R4_OFFSET)];

    th_trace("rpi_fork: tid=%d, code=[%p], arg=[%x], saved_sp=[%p]\n",
            t->tid, code, arg, t->saved_sp);

    Q_append(&runq, t);
    return t;
}


// exit current thread.
//   - if no more threads, switch to the scheduler.
//   - otherwise context switch to the new thread.
//     make sure to set cur_thread correctly!
void rpi_exit(int exitcode) {
    redzone_check(0);

    // If empty -- switch from curr thread to scheduler, and free
    if (Q_empty(&runq)) {
        th_free(cur_thread);

        // Make sure to trace, as well!
        th_trace("done running threads, back to scheduler\n");
        rpi_cswitch(&cur_thread->saved_sp, scheduler_thread->saved_sp);
    } else {
        // Else, we get the new thread, and set to cur_thread
        rpi_thread_t *old_thread = cur_thread;
        cur_thread = Q_pop(&runq);

        // Context switch to the new cur_thread
        th_free(old_thread);
        rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
    }

    // should never return.
    not_reached();
}

// yield the current thread.
//   - if the runq is empty, return.
//   - otherwise: 
//      * add the current thread to the back 
//        of the runq (Q_append)
//      * context switch to the new thread.
//        make sure to set cur_thread correctly!
void rpi_yield(void) {
    redzone_check(0);

    // If runq is empty, just return
    if (Q_empty(&runq)) {
        return;
    }

    // Add current thread to the back of the runq
    Q_append(&runq, cur_thread);
    rpi_thread_t *old_thread = cur_thread;

    // Context switch to the new thread (and set cur_thread correctly!)
    cur_thread = Q_pop(&runq);
    th_trace("switching from tid=%d to tid=%d\n", old_thread->tid, cur_thread->tid);
    rpi_cswitch(&old_thread->saved_sp, cur_thread->saved_sp);
}

/*
 * starts the thread system.  
 * note: our caller is not a thread!  so you have to 
 * create a fake thread (assign it to scheduler_thread)
 * so that context switching works correctly.   your code
 * should work even if the runq is empty.
 */
void rpi_thread_start(void) {
    th_trace("starting threads!\n");

    // no other runnable thread: return.
    if(Q_empty(&runq))
        goto end;

    // setup scheduler thread block.
    if(!scheduler_thread)
        scheduler_thread = th_alloc();

    // Pop and then switch -- don't free!
    cur_thread = Q_pop(&runq);
    rpi_cswitch(&scheduler_thread->saved_sp, cur_thread->saved_sp);


end:
    redzone_check(0);
    // if not more threads should print:
    th_trace("done with all threads, returning\n");
}

// helper routine: can call from assembly with r0=sp and it
// will print the stack out.  it then exits.
// call this if you can't figure out what is going on in your
// assembly.
void rpi_print_regs(uint32_t *sp) {
    // use this to check that your offsets are correct.
    printk("cur-thread=%d\n", cur_thread->tid);
    printk("sp=%p\n", sp);

    // stack pointer better be between these.
    printk("stack=%p\n", &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp < &cur_thread->stack[THREAD_MAXSTACK]);
    assert(sp >= &cur_thread->stack[0]);
    for(unsigned i = 0; i < 9; i++) {
        unsigned r = i == 8 ? 14 : i + 4;
        printk("sp[%d]=r%d=%x\n", i, r, sp[i]);
    }
    clean_reboot();
}
