#include "doctest.h"
#include "tactility_core.h"
#include "Mutex.h"

using namespace tt;

static int thread_with_mutex_parameter(void* parameter) {
    auto* mutex = (Mutex*)parameter;
    tt_mutex_acquire(mutex, TtWaitForever);
    return 0;
}

TEST_CASE("a mutex can block a thread") {
    auto* mutex = tt_mutex_alloc(MutexTypeNormal);
    tt_mutex_acquire(mutex, TtWaitForever);

    Thread* thread = thread_alloc_ex(
        "thread",
        1024,
        &thread_with_mutex_parameter,
        mutex
    );
    thread_start(thread);

    delay_ms(5);
    CHECK_EQ(thread_get_state(thread), ThreadStateRunning);

    tt_mutex_release(mutex);

    delay_ms(5);
    CHECK_EQ(thread_get_state(thread), ThreadStateStopped);

    thread_join(thread);
    thread_free(thread);
    tt_mutex_free(mutex);
}
