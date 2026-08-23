#include "doctest.h"

#include <app/event.h>

#include <tactility/concurrent/thread.h>
#include <tactility/delay.h>
#include <tactility/time.h>

TEST_CASE("app_event_subscribe/_poll deliver events in FIFO order") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = 1;
    CHECK_EQ(app_event_subscribe(&sub, &event_group), ERROR_NONE);

    for (uint32_t i = 0; i < 3; i++) {
        AppEvent event { .type = APP_EVENT_RESULT, .timestamp = 0, .result = { .launch_id = i, .result = 0 } };
        CHECK_EQ(app_event_emit(1, &event), ERROR_NONE);
    }

    for (uint32_t i = 0; i < 3; i++) {
        AppEvent out {};
        CHECK_EQ(app_event_poll(&sub, &out), ERROR_NONE);
        CHECK_EQ(out.type, APP_EVENT_RESULT);
        CHECK_EQ(out.result.launch_id, i);
    }

    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("app_event_emit only delivers to subscriptions for that app_instance_id") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = 10;
    app_event_subscribe(&sub, &event_group);

    AppEvent event { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
    CHECK_EQ(app_event_emit(11, &event), ERROR_NOT_FOUND);

    AppEvent out {};
    CHECK_EQ(app_event_poll(&sub, &out), ERROR_TIMEOUT);

    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("app_event_emit returns ERROR_RESOURCE and drops the newest event once a subscription's queue is full") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = 20;
    app_event_subscribe(&sub, &event_group);

    for (uint32_t i = 0; i < APP_EVENT_QUEUE_CAPACITY; i++) {
        AppEvent event { .type = APP_EVENT_RESULT, .timestamp = 0, .result = { .launch_id = i, .result = 0 } };
        CHECK_EQ(app_event_emit(20, &event), ERROR_NONE);
    }

    // Queue is now full; this one should be dropped.
    AppEvent overflow_event { .type = APP_EVENT_RESULT, .timestamp = 0, .result = { .launch_id = 999, .result = 0 } };
    CHECK_EQ(app_event_emit(20, &overflow_event), ERROR_RESOURCE);

    // The already-queued events survive, in order, and the dropped one never arrives.
    for (uint32_t i = 0; i < APP_EVENT_QUEUE_CAPACITY; i++) {
        AppEvent out {};
        CHECK_EQ(app_event_poll(&sub, &out), ERROR_NONE);
        CHECK_EQ(out.result.launch_id, i);
    }
    AppEvent out {};
    CHECK_EQ(app_event_poll(&sub, &out), ERROR_TIMEOUT);

    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("app_event_unsubscribe stops further delivery") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = 30;
    app_event_subscribe(&sub, &event_group);

    CHECK_EQ(app_event_unsubscribe(&sub), ERROR_NONE);
    CHECK_EQ(app_event_unsubscribe(&sub), ERROR_NOT_FOUND);

    AppEvent event { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
    CHECK_EQ(app_event_emit(30, &event), ERROR_NOT_FOUND);

    task_event_group_destruct(&event_group);
}

TEST_CASE("app_event_poll times out when no event has arrived") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = 40;
    app_event_subscribe(&sub, &event_group);

    AppEvent out {};
    CHECK_EQ(app_event_poll(&sub, &out), ERROR_TIMEOUT);

    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("app_event_emit stamps the event with the current boot-relative time") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = 50;
    app_event_subscribe(&sub, &event_group);

    auto before = static_cast<uint64_t>(get_micros_since_boot());
    AppEvent event { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
    app_event_emit(50, &event);
    auto after = static_cast<uint64_t>(get_micros_since_boot());

    AppEvent out {};
    REQUIRE_EQ(app_event_poll(&sub, &out), ERROR_NONE);
    CHECK_GE(out.timestamp, before);
    CHECK_LE(out.timestamp, after);

    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("task_event_group_wait wakes when the event is emitted from another task") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    AppEventSubscription sub {};
    sub.app_instance_id = 60;
    CHECK_EQ(app_event_subscribe(&sub, &event_group), ERROR_NONE);

    auto* thread = thread_alloc_full(
        "app-event-emitter",
        4096,
        [](void*) -> int32_t {
            delay_millis(20);
            AppEvent event { .type = APP_EVENT_CLOSE, .timestamp = 0, .result = {} };
            app_event_emit(60, &event);
            return 0;
        },
        nullptr,
        -1
    );
    CHECK_EQ(thread_start(thread), ERROR_NONE);

    CHECK_EQ(task_event_group_wait(&event_group, sub.bit, false, nullptr, pdMS_TO_TICKS(2000)), ERROR_NONE);
    AppEvent out {};
    CHECK_EQ(app_event_poll(&sub, &out), ERROR_NONE);
    CHECK_EQ(out.type, APP_EVENT_CLOSE);

    CHECK_EQ(thread_join(thread, pdMS_TO_TICKS(2000), 1), ERROR_NONE);
    thread_free(thread);

    app_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}
