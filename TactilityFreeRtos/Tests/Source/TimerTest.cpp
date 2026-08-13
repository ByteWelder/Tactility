#include "doctest.h"
#include <Tactility/Timer.h>

#include <atomic>

using namespace tt;

// Timer::stop() can return while a just-triggered callback is still executing (documented on
// Timer::stop() itself) - this grace delay gives that in-flight callback a chance to finish
// before the test deletes the timer and its captured state goes out of scope.
constexpr TickType_t STOP_GRACE_TICKS = 5;

TEST_CASE("TimerType::Periodic timers can be stopped and restarted") {
    std::atomic<int> counter{0};
    auto* timer = new Timer(Timer::Type::Periodic, 1, [&counter] { counter++; });
    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(10);
    CHECK_EQ(timer->stop(), true);
    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(10);
    CHECK_EQ(timer->stop(), true);
    kernel::delayTicks(STOP_GRACE_TICKS);
    delete timer;

    CHECK_GE(counter.load(), 2);
}

TEST_CASE("TimerType::Periodic calls the callback periodically") {
    int ticks_to_run = 10;
    std::atomic<int> counter{0};
    auto* timer = new Timer(Timer::Type::Periodic, 1, [&counter] { counter++; });
    CHECK_EQ(timer->start(), true);
    kernel::delayTicks(ticks_to_run);
    CHECK_EQ(timer->stop(), true);
    kernel::delayTicks(STOP_GRACE_TICKS);
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
    kernel::delayTicks(STOP_GRACE_TICKS);
    delete timer;

    CHECK_EQ(counter.load(), 2);
}
