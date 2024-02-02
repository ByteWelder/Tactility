#include "doctest.h"
#include "tactility_core.h"
#include "timer.h"


void* timer_callback_context = NULL;
static void timer_callback_with_context(void* context) {
    timer_callback_context = context;
}

static void timer_callback_with_counter(void* context) {
    int* int_ptr = (int*)context;
    (*int_ptr)++;
}

TEST_CASE("a timer passes the context correctly") {
    int foo = 1;
    Timer* timer = tt_timer_alloc(&timer_callback_with_context, TimerTypeOnce, &foo);
    tt_timer_start(timer, 1);
    tt_delay_tick(10);
    tt_timer_stop(timer);
    tt_timer_free(timer);

    CHECK_EQ(timer_callback_context, &foo);
}

TEST_CASE("TimerTypePeriodic timers can be stopped and restarted") {
    int counter = 0;
    Timer* timer = tt_timer_alloc(&timer_callback_with_counter, TimerTypePeriodic, &counter);
    tt_timer_start(timer, 1);
    tt_delay_tick(10);
    tt_timer_stop(timer);
    tt_delay_tick(10);
    tt_timer_stop(timer);
    tt_timer_free(timer);

    CHECK_GE(counter, 2);
}

TEST_CASE("TimerTypePeriodic calls the callback periodically") {
    int counter = 0;
    int ticks_to_run = 10;
    Timer* timer = tt_timer_alloc(&timer_callback_with_counter, TimerTypePeriodic, &counter);
    tt_timer_start(timer, 1);
    tt_delay_tick(ticks_to_run);
    tt_timer_stop(timer);
    tt_timer_free(timer);

    CHECK_EQ(counter, ticks_to_run);
}

TEST_CASE("restarting TimerTypeOnce timers has no effect") {
    int counter = 0;
    Timer* timer = tt_timer_alloc(&timer_callback_with_counter, TimerTypeOnce, &counter);
    tt_timer_start(timer, 1);
    tt_delay_tick(10);
    tt_timer_stop(timer);
    tt_delay_tick(10);
    tt_timer_stop(timer);
    tt_timer_free(timer);

    CHECK_EQ(counter, 1);
}
