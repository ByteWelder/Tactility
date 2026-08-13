#include "doctest.h"
#include <Tactility/Semaphore.h>
#include <Tactility/Timer.h>

#include <atomic>

using namespace tt;

// stop() only enqueues a command; it doesn't wait for the timer service task to process it or
// finish an in-flight callback. Queuing a pending callback on that same queue and waiting for it
// does, since the service task processes its queue in order.
void waitForTimerServiceIdle(Timer& timer) {
    Semaphore done(1, 0);
    auto markDone = [](void* context, uint32_t) {
        static_cast<Semaphore*>(context)->release();
    };
    REQUIRE(timer.setPendingCallback(markDone, &done, 0, pdMS_TO_TICKS(2000)));
    REQUIRE(done.acquire(pdMS_TO_TICKS(2000)));
}

TEST_CASE("TimerType::Periodic timers can be stopped and restarted") {
    std::atomic<int> counter{0};
    auto* timer = new Timer(Timer::Type::Periodic, 1, [&counter] { counter++; });
    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(10);
    CHECK_EQ(timer->stop(), true);
    waitForTimerServiceIdle(*timer);
    const auto first_run_count = counter.load();

    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(10);
    CHECK_EQ(timer->stop(), true);
    waitForTimerServiceIdle(*timer);
    delete timer;

    CHECK_GT(counter.load(), first_run_count);
}

TEST_CASE("TimerType::Periodic calls the callback periodically") {
    int ticks_to_run = 10;
    std::atomic<int> counter{0};
    auto* timer = new Timer(Timer::Type::Periodic, 1, [&counter] { counter++; });
    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(ticks_to_run);
    CHECK_EQ(timer->stop(), true);
    waitForTimerServiceIdle(*timer);
    delete timer;

    // Exact count isn't guaranteed (scheduling slop around start()/stop()), so this only checks
    // that the callback fired repeatedly, not an exact tick-for-tick match.
    CHECK_GE(counter.load(), ticks_to_run / 2);
}

TEST_CASE("restarting TimerType::Once timers calls the callback again") {
    std::atomic<int> counter{0};
    auto* timer = new Timer(Timer::Type::Once, 1, [&counter] { counter++; });
    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(10);
    CHECK_EQ(timer->stop(), true);
    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(10);
    CHECK_EQ(timer->stop(), true);
    waitForTimerServiceIdle(*timer);
    delete timer;

    CHECK_EQ(counter.load(), 2);
}
