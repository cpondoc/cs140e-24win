// write code in C to check if stack grows up or down.
// suggestion:
//   - local variables are on the stack.
//   - so take the address of a local, call another routine, and
//     compare addresses.
//   - make sure you check the machine code make sure the
//     compiler didn't optimize the calls away.
#include "rpi.h"

void recurse(int n) {
    int x = n;
    trace("%p\n", &x);
    if (n == 0) {}
    else {
        recurse(n - 1);
    }
}

// News flash -- stack grows down!
int stack_grows_down(void) {
    recurse(10);
    return 1;

}

void notmain(void) {
    if(stack_grows_down())
        trace("stack grows down\n");
    else
        trace("stack grows up\n");
}
