#include "doctest.h"
#include <Tactility/DispatcherThread.h>
#include <Tactility/Semaphore.h>

using namespace tt;

TEST_CASE("DispatcherThread state test") {
    DispatcherThread thread("test");
    CHECK_EQ(thread.isStarted(), false);

    thread.start();
    CHECK_EQ(thread.isStarted(), true);

    thread.stop();
    CHECK_EQ(thread.isStarted(), false);
}

TEST_CASE("DispatcherThread should consume jobs") {
    DispatcherThread thread("test");
    thread.start();
    int counter = 0;
    Semaphore done(1, 0);

    thread.dispatch([&counter, &done]() {
        counter++;
        done.release();
    });

    CHECK(done.acquire(pdMS_TO_TICKS(2000)));
    CHECK_EQ(counter, 1);
    thread.stop();
}
