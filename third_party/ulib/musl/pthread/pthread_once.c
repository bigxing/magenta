#include "futex_impl.h"
#include "pthread_impl.h"

static void undo(void* control) {
    atomic_int* futex = control;

    /* Wake all waiters, since the waiter status is lost when
     * resetting control to the initial state. */
    if (atomic_exchange(futex, 0) == 3)
        __wake(futex, -1);
}

int __pthread_once_full(pthread_once_t* control, void (*init)(void)) {
    /* Try to enter initializing state. Four possibilities:
     *  0 - we're the first or the other cancelled; run init
     *  1 - another thread is running init; wait
     *  2 - another thread finished running init; just return
     *  3 - another thread is running init, waiters present; wait */

    for (;;)
        switch (a_cas_shim(control, 0, 1)) {
        case 0:
            pthread_cleanup_push(undo, control);
            init();
            pthread_cleanup_pop(0);

            if (atomic_exchange(control, 2) == 3)
                __wake(control, -1);
            return 0;
        case 1:
            /* If this fails, so will __wait. */
            a_cas_shim(control, 1, 3);
        case 3:
            __wait(control, 0, 3);
            continue;
        case 2:
            return 0;
        }
}

int __pthread_once(pthread_once_t* control, void (*init)(void)) {
    /* Return immediately if init finished before, but ensure that
     * effects of the init routine are visible to the caller. */
    if (atomic_load(control) == 2) {
        atomic_thread_fence(memory_order_seq_cst);
        return 0;
    }
    return __pthread_once_full(control, init);
}

weak_alias(__pthread_once, pthread_once);
