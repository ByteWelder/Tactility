#include "doctest.h"

#include <tactility/system_event.h>
#include <tactility/time.h>

#include <vector>

// system_event_emit() snapshots matching subscriptions under the lock, then invokes them
// after unlocking (see the @warning on system_event_subscribe() in system_event.h), so a
// callback calling system_event_subscribe()/_unsubscribe()/_emit() must not deadlock -
// covered below, mirroring DeviceListenerTest.cpp's reentrancy test.

struct RecordedCall {
    void* context;
    SystemEventType type;
    const void* data;
    size_t data_len;
    uint64_t timestamp;
};

static std::vector<RecordedCall> calls_a;
static std::vector<RecordedCall> calls_b;

static void listener_a(SystemEvent* event, void* context) {
    calls_a.push_back({ context, event->type, event->data, event->data_len, event->timestamp });
}

static void listener_b(SystemEvent* event, void* context) {
    calls_b.push_back({ context, event->type, event->data, event->data_len, event->timestamp });
}

static void reset_calls() {
    calls_a.clear();
    calls_b.clear();
}

TEST_CASE("system_event_emit invokes every subscriber registered for that type") {
    reset_calls();
    int context_a = 1;
    int context_b = 2;

    CHECK_EQ(system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a), ERROR_NONE);
    CHECK_EQ(system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b, &context_b), ERROR_NONE);

    CHECK_EQ(system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0), ERROR_NONE);

    REQUIRE_EQ(calls_a.size(), 1);
    CHECK_EQ(calls_a[0].context, &context_a);
    CHECK_EQ(calls_a[0].type, KERNEL_EVENT_BOOT_COMPLETED);

    REQUIRE_EQ(calls_b.size(), 1);
    CHECK_EQ(calls_b[0].context, &context_b);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
}

TEST_CASE("system_event_emit only invokes subscribers registered for the emitted type") {
    reset_calls();
    int context_a = 1;
    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);

    system_event_emit(KERNEL_EVENT_TIME_CHANGED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 0);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 1);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
}

TEST_CASE("system_event_emit passes the data pointer and length through unchanged") {
    reset_calls();
    int context_a = 1;
    struct Payload { int value; } payload { 42 };

    system_event_subscribe(KERNEL_EVENT_TIME_CHANGED, listener_a, &context_a);
    system_event_emit(KERNEL_EVENT_TIME_CHANGED, &payload, sizeof(payload));

    REQUIRE_EQ(calls_a.size(), 1);
    CHECK_EQ(calls_a[0].data, &payload);
    CHECK_EQ(calls_a[0].data_len, sizeof(payload));
    CHECK_EQ(static_cast<const Payload*>(calls_a[0].data)->value, 42);

    system_event_unsubscribe(KERNEL_EVENT_TIME_CHANGED, listener_a);
}

TEST_CASE("system_event_emit with no data passes a null pointer and zero length") {
    reset_calls();
    int context_a = 1;
    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);

    REQUIRE_EQ(calls_a.size(), 1);
    CHECK_EQ(calls_a[0].data, nullptr);
    CHECK_EQ(calls_a[0].data_len, 0);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
}

TEST_CASE("system_event_unsubscribe stops further notifications for that callback only") {
    reset_calls();
    int context_a = 1;
    int context_b = 2;

    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);
    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b, &context_b);

    CHECK_EQ(system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a), ERROR_NONE);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);

    CHECK_EQ(calls_a.size(), 0);
    CHECK_EQ(calls_b.size(), 1);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
}

TEST_CASE("system_event_unsubscribe on an unregistered callback returns ERROR_NOT_FOUND and is a no-op") {
    reset_calls();
    int context_b = 2;
    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b, &context_b);

    // listener_a was never added for this type, so removing it must not disturb listener_b.
    CHECK_EQ(system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a), ERROR_NOT_FOUND);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    CHECK_EQ(calls_b.size(), 1);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
}

TEST_CASE("system_event_unsubscribe matches on (type, callback), not the callback alone") {
    reset_calls();
    int context_a = 1;

    // Same callback subscribed for two different event types.
    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);
    system_event_subscribe(KERNEL_EVENT_TIME_CHANGED, listener_a, &context_a);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 0);

    system_event_emit(KERNEL_EVENT_TIME_CHANGED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 1);

    system_event_unsubscribe(KERNEL_EVENT_TIME_CHANGED, listener_a);
}

TEST_CASE("system_event_emit with no subscribers for that type returns ERROR_NONE") {
    CHECK_EQ(system_event_emit(KERNEL_EVENT_SERVICE_STOPPED, nullptr, 0), ERROR_NONE);
}

TEST_CASE("system_event_emit stamps the event with the current boot-relative time") {
    reset_calls();
    int context_a = 1;
    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);

    auto before = static_cast<uint64_t>(get_micros_since_boot());
    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    auto after = static_cast<uint64_t>(get_micros_since_boot());

    REQUIRE_EQ(calls_a.size(), 1);
    CHECK_GE(calls_a[0].timestamp, before);
    CHECK_LE(calls_a[0].timestamp, after);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
}

static bool reentrant_add_triggered = false;

static void reentrant_listener(SystemEvent* event, void* context) {
    calls_a.push_back({ context, event->type, event->data, event->data_len, event->timestamp });
    if (!reentrant_add_triggered) {
        reentrant_add_triggered = true;
        // Subscribing from within a notification must not deadlock: emit() releases the
        // lock before invoking callbacks, so this only blocks briefly on the (already
        // unlocked) mutex.
        system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b, context);
        // Also exercise unsubscribe() and a nested emit() of a different type from within
        // a callback - all must complete without deadlocking.
        system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, reentrant_listener);
        system_event_emit(KERNEL_EVENT_TIME_CHANGED, nullptr, 0);
    }
}

TEST_CASE("system_event_emit is safe when a callback subscribes, unsubscribes and emits during notification") {
    reset_calls();
    reentrant_add_triggered = false;
    int context_a = 1;

    system_event_subscribe(KERNEL_EVENT_BOOT_COMPLETED, reentrant_listener, &context_a);
    system_event_subscribe(KERNEL_EVENT_TIME_CHANGED, listener_b, &context_a);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);

    // reentrant_listener unsubscribed itself and triggered a nested TIME_CHANGED emit,
    // which the pre-existing listener_b subscription picks up. The listener_b
    // subscription added *during* this round wasn't part of this round's snapshot, so it
    // wasn't invoked for BOOT_COMPLETED yet.
    CHECK_EQ(calls_a.size(), 1);
    CHECK_EQ(calls_b.size(), 1);

    // A second BOOT_COMPLETED emit must not reach reentrant_listener again (it
    // unsubscribed itself), but must reach the listener_b subscription added last round.
    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 1);
    CHECK_EQ(calls_b.size(), 2);

    system_event_unsubscribe(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
    system_event_unsubscribe(KERNEL_EVENT_TIME_CHANGED, listener_b);
}
