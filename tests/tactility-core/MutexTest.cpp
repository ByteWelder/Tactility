#include "doctest.h"
#include "tactility_core.h"
#include "Mutex.h"

static int thread_with_mutex_parameter(void* parameter) {
    auto* mutex = (Mutex*)parameter;
    tt_mutex_acquire(mutex, TtWaitForever);
    return 0;
}

TEST_CASE("a mutex can block a thread") {
    auto* mutex = tt_mutex_alloc(MutexTypeNormal);
    tt_mutex_acquire(mutex, TtWaitForever);

    Thread* thread = tt_thread_alloc_ex(
        "thread",
        1024,
        &thread_with_mutex_parameter,
        mutex
    );
    tt_thread_start(thread);

    tt_delay_ms(5);
    CHECK_EQ(tt_thread_get_state(thread), ThreadStateRunning);

    tt_mutex_release(mutex);

    tt_delay_ms(5);
    CHECK_EQ(tt_thread_get_state(thread), ThreadStateStopped);

    tt_thread_join(thread);
    tt_thread_free(thread);
    tt_mutex_free(mutex);
}
