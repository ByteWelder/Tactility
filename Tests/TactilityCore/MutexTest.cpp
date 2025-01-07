#include "doctest.h"
#include "TactilityCore.h"
#include "Mutex.h"

using namespace tt;

static int32_t thread_with_mutex_parameter(void* parameter) {
    auto* mutex = (Mutex*)parameter;
    mutex->lock(portMAX_DELAY);
    return 0;
}

TEST_CASE("a mutex can block a thread") {
    auto mutex = Mutex(Mutex::TypeNormal);
    mutex.lock(portMAX_DELAY);

    Thread thread = Thread(
        "thread",
        1024,
        &thread_with_mutex_parameter,
        &mutex
    );
    thread.start();

    kernel::delayMillis(5);
    CHECK_EQ(thread.getState(), Thread::StateRunning);

    mutex.unlock();

    kernel::delayMillis(5);
    CHECK_EQ(thread.getState(), Thread::StateStopped);

    thread.join();
}
