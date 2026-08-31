#include <pthread.h>
#include <stddef.h>

/**
 * Linked in with -Wl,--wrap=pthread_attr_setstack (see Tactility/CMakeLists.txt).
 *
 * FreeRTOS's POSIX port (Libraries/FreeRTOS-Kernel/portable/ThirdParty/GCC/Posix/port.c)
 * hands every task a stack carved out of its own heap (pvPortMalloc) via this call.
 * pthread_attr_setstack requires page alignment, which pvPortMalloc doesn't guarantee;
 * when it happens to succeed anyway (allocator alignment can vary run to run), the
 * task's real pthread stack ends up living inside that small FreeRTOS heap region -
 * fine for typical embedded task code, but a desktop GL driver doing on-the-fly shader
 * compilation on that thread (e.g. Mesa on first frame present) can overflow it and
 * silently corrupt adjacent heap_4 objects.
 *
 * Wrapping the call out entirely (rather than patching the vendored port.c) leaves every
 * task's pthread_attr_t at its pthread_attr_init() default, so pthread_create() always
 * gives it a real, properly allocated default-size stack instead.
 */
int __wrap_pthread_attr_setstack(pthread_attr_t* attr, void* stackaddr, size_t stacksize) {
    (void)attr;
    (void)stackaddr;
    (void)stacksize;
    return 0;
}
