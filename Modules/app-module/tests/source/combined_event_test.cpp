#include "doctest.h"

#include <app/event.h>

#include <tactility/system_event.h>

// Regression coverage for the primary motivation behind TaskEventGroup: a task subscribed to
// both an app_event and a system_event must be able to block once and wake for either, without
// losing an event or regressing either subsystem's own delivery semantics (FIFO for app_event,
// coalescing for system_event).

TEST_CASE("a task can wait on app_event and system_event together via one TaskEventGroup") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription app_sub {};
    CHECK_EQ(app_event_subscribe_with_app_id(&app_sub, &event_group, 100), ERROR_NONE);

    SystemEventSubscription sys_sub {};
    sys_sub.event.type = KERNEL_EVENT_BOOT_COMPLETED;
    CHECK_EQ(system_event_subscribe(&sys_sub, &event_group), ERROR_NONE);

    // Distinct bits, so a combined wait can tell (via out_flags) which source(s) fired.
    CHECK_NE(app_sub.bit, sys_sub.bit);

    uint32_t both_bits = app_sub.bit | sys_sub.bit;

    // Only app_event fires: combined wait matches just that bit, and only app_event_poll()
    // finds something to pop.
    AppEvent emitted { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
    CHECK_EQ(app_event_emit(100, &emitted), ERROR_NONE);

    uint32_t out_flags = 0;
    CHECK_EQ(task_event_group_wait(&event_group, both_bits, false, &out_flags, 0), ERROR_NONE);
    CHECK_EQ(out_flags, app_sub.bit);

    AppEvent app_out {};
    CHECK_EQ(app_event_poll(&app_sub, &app_out), ERROR_NONE);
    CHECK_EQ(app_out.type, APP_EVENT_CLOSE);
    CHECK_EQ(system_event_poll(&sys_sub), ERROR_TIMEOUT);

    // Only system_event fires: combined wait matches just that bit, and only
    // system_event_poll() finds something pending.
    CHECK_EQ(system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0), ERROR_NONE);

    out_flags = 0;
    CHECK_EQ(task_event_group_wait(&event_group, both_bits, false, &out_flags, 0), ERROR_NONE);
    CHECK_EQ(out_flags, sys_sub.bit);

    CHECK_EQ(system_event_poll(&sys_sub), ERROR_NONE);
    AppEvent app_out2 {};
    CHECK_EQ(app_event_poll(&app_sub, &app_out2), ERROR_TIMEOUT);

    // Both fire before the wait: combined wait matches both bits, and both subsystems' own
    // polls (which re-check their own state rather than trusting the bit) still deliver.
    CHECK_EQ(app_event_emit(100, &emitted), ERROR_NONE);
    CHECK_EQ(system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0), ERROR_NONE);

    out_flags = 0;
    CHECK_EQ(task_event_group_wait(&event_group, both_bits, false, &out_flags, 0), ERROR_NONE);
    CHECK_EQ(out_flags, both_bits);

    CHECK_EQ(app_event_poll(&app_sub, &app_out), ERROR_NONE);
    CHECK_EQ(system_event_poll(&sys_sub), ERROR_NONE);

    // Unsubscribe order (system_event before app_event) must not matter - the group outlives
    // both and is destructed last.
    system_event_unsubscribe(&sys_sub);
    app_event_unsubscribe(&app_sub);
    task_event_group_destruct(&event_group);
}
