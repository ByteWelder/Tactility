#include "doctest.h"
#include "TactilityCore.h"
#include "Mutex.h"

using namespace tt;

static int thread_with_mutex_parameter(void* parameter) {
    auto* mutex = (Mutex*)parameter;
    tt_mutex_acquire(mutex, TtWaitForever);
    return 0;
}

TEST_CASE("a mutex can block a thread") {
    auto* mutex = tt_mutex_alloc(Mutex::TypeNormal);
    tt_mutex_acquire(mutex, TtWaitForever);

    Thread* thread = new Thread(
        "thread",
        1024,
        &thread_with_mutex_parameter,
        mutex
    );
    thread->start();

    delay_ms(5);
    CHECK_EQ(thread->getState(), Thread::StateRunning);

    tt_mutex_release(mutex);

    delay_ms(5);
    CHECK_EQ(thread->getState(), Thread::StateStopped);

    thread->join();
    delete thread;
    tt_mutex_free(mutex);
}
