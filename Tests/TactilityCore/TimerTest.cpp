#include "doctest.h"
#include <Tactility/TactilityCore.h>
#include <Tactility/Timer.h>

using namespace tt;

TEST_CASE("TimerType::Periodic timers can be stopped and restarted") {
    int counter = 0;
    auto* timer = new Timer(Timer::Type::Periodic, [&counter]() { counter++; });
    timer->start(1);
    kernel::delayTicks(10);
    timer->stop();
    timer->start(1);
    kernel::delayTicks(10);
    timer->stop();
    delete timer;

    CHECK_GE(counter, 2);
}

TEST_CASE("TimerType::Periodic calls the callback periodically") {
    int ticks_to_run = 10;
    int counter = 0;
    auto* timer = new Timer(Timer::Type::Periodic, [&counter]() { counter++; });
    timer->start(1);
    kernel::delayTicks(ticks_to_run);
    timer->stop();
    delete timer;

    CHECK_EQ(counter, ticks_to_run);
}

TEST_CASE("restarting TimerType::Once timers calls the callback again") {
    int counter = 0;
    auto* timer = new Timer(Timer::Type::Once, [&counter]() { counter++; });
    timer->start(1);
    kernel::delayTicks(10);
    timer->stop();
    timer->start(1);
    kernel::delayTicks(10);
    timer->stop();
    delete timer;

    CHECK_EQ(counter, 2);
}
