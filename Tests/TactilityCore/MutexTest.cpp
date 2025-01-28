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
    auto mutex = Mutex(Mutex::Type::Normal);
    mutex.lock(portMAX_DELAY);

    Thread thread = Thread(
        "thread",
        1024,
        &thread_with_mutex_parameter,
        &mutex
    );
    thread.start();

    kernel::delayMillis(5);
    CHECK_EQ(thread.getState(), Thread::State::Running);

    mutex.unlock();

    kernel::delayMillis(5);
    CHECK_EQ(thread.getState(), Thread::State::Stopped);

    thread.join();
}

TEST_CASE("a Mutex can be locked exactly once") {
    auto mutex = Mutex(Mutex::Type::Normal);
    CHECK_EQ(mutex.lock(0), true);
    CHECK_EQ(mutex.lock(0), false);
    CHECK_EQ(mutex.unlock(), true);
}

TEST_CASE("unlocking a Mutex without locking returns false") {
    auto mutex = Mutex(Mutex::Type::Normal);
    CHECK_EQ(mutex.unlock(), false);
}

TEST_CASE("unlocking a Mutex twice returns false on the second attempt") {
    auto mutex = Mutex(Mutex::Type::Normal);
    CHECK_EQ(mutex.lock(0), true);
    CHECK_EQ(mutex.unlock(), true);
    CHECK_EQ(mutex.unlock(), false);
}
