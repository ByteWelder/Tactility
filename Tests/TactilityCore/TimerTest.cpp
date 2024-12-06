#include "doctest.h"
#include "TactilityCore.h"
#include "Timer.h"

#include <utility>

using namespace tt;

std::shared_ptr<void> timer_callback_context = NULL;
static void timer_callback_with_context(std::shared_ptr<void> context) {
    timer_callback_context = std::move(context);
}

static void timer_callback_with_counter(std::shared_ptr<void> context) {
    auto int_ptr = std::static_pointer_cast<int>(context);
    (*int_ptr)++;
}

TEST_CASE("a timer passes the context correctly") {
    auto foo = std::make_shared<int>(1);
    auto* timer = new Timer(Timer::TypeOnce, &timer_callback_with_context, foo);
    timer->start(1);
    delay_ticks(10);
    timer->stop();
    delete timer;

    CHECK_EQ(*std::static_pointer_cast<int>(timer_callback_context), *foo);
}

TEST_CASE("TimerTypePeriodic timers can be stopped and restarted") {
    auto counter = std::make_shared<int>(0);
    auto* timer = new Timer(Timer::TypePeriodic, &timer_callback_with_counter, counter);
    timer->start(1);
    delay_ticks(10);
    timer->stop();
    timer->start(1);
    delay_ticks(10);
    timer->stop();
    delete timer;

    CHECK_GE(*counter, 2);
}

TEST_CASE("TimerTypePeriodic calls the callback periodically") {
    int counter = 0;
    int ticks_to_run = 10;
    auto* timer = new Timer(Timer::TypePeriodic, &timer_callback_with_counter, &counter);
    timer->start(1);
    delay_ticks(ticks_to_run);
    timer->stop();
    delete timer;

    CHECK_EQ(counter, ticks_to_run);
}

TEST_CASE("restarting TimerTypeOnce timers calls the callback again") {
    int counter = 0;
    auto* timer = new Timer(Timer::TypeOnce, &timer_callback_with_counter, &counter);
    timer->start(1);
    delay_ticks(10);
    timer->stop();
    timer->start(1);
    delay_ticks(10);
    timer->stop();
    delete timer;

    CHECK_EQ(counter, 2);
}
