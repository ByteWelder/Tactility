#include "doctest.h"

#include <tactility/concurrent/thread.h>
#include <tactility/delay.h>
#include <tactility/system_event.h>
#include <tactility/time.h>

#include <vector>

// system_event_emit() snapshots matching subscriptions under the lock, then invokes them
// after unlocking (see the @warning on system_event_callback_add() in system_event.h), so a
// callback calling system_event_callback_add()/_unsubscribe()/_emit() must not deadlock -
// covered below, mirroring DeviceListenerTest.cpp's reentrancy test.

struct RecordedCall {
    void* context;
    SystemEventType type;
    // Copied out of event->data during the callback - event->data is only valid for the
    // duration of the callback (it lives in system_event_emit()'s own stack frame), so a bare
    // pointer/length pair recorded here would dangle by the time a TEST_CASE inspects it.
    std::vector<uint8_t> data;
    uint64_t timestamp;
};

static std::vector<RecordedCall> calls_a;
static std::vector<RecordedCall> calls_b;

static void listener_a(SystemEvent* event, void* context) {
    calls_a.push_back({ context, event->type, std::vector<uint8_t>(event->data, event->data + event->data_len), event->timestamp });
}

static void listener_b(SystemEvent* event, void* context) {
    calls_b.push_back({ context, event->type, std::vector<uint8_t>(event->data, event->data + event->data_len), event->timestamp });
}

static void reset_calls() {
    calls_a.clear();
    calls_b.clear();
}

TEST_CASE("system_event_emit invokes every subscriber registered for that type") {
    reset_calls();
    int context_a = 1;
    int context_b = 2;

    CHECK_EQ(system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a), ERROR_NONE);
    CHECK_EQ(system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_b, &context_b), ERROR_NONE);

    CHECK_EQ(system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0), ERROR_NONE);

    REQUIRE_EQ(calls_a.size(), 1);
    CHECK_EQ(calls_a[0].context, &context_a);
    CHECK_EQ(calls_a[0].type, KERNEL_EVENT_BOOT_COMPLETED);

    REQUIRE_EQ(calls_b.size(), 1);
    CHECK_EQ(calls_b[0].context, &context_b);

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
}

TEST_CASE("system_event_emit only invokes subscribers registered for the emitted type") {
    reset_calls();
    int context_a = 1;
    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);

    system_event_emit(KERNEL_EVENT_TIME_CHANGED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 0);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 1);

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
}

TEST_CASE("system_event_emit copies the data into the delivered event") {
    reset_calls();
    int context_a = 1;
    struct Payload { int value; } payload { 42 };

    system_event_callback_add(KERNEL_EVENT_TIME_CHANGED, listener_a, &context_a);
    system_event_emit(KERNEL_EVENT_TIME_CHANGED, &payload, sizeof(payload));

    REQUIRE_EQ(calls_a.size(), 1);
    REQUIRE_EQ(calls_a[0].data.size(), sizeof(payload));
    CHECK_EQ(reinterpret_cast<const Payload*>(calls_a[0].data.data())->value, 42);

    system_event_callback_remove(KERNEL_EVENT_TIME_CHANGED, listener_a);
}

TEST_CASE("system_event_emit with no data delivers an empty payload") {
    reset_calls();
    int context_a = 1;
    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);

    REQUIRE_EQ(calls_a.size(), 1);
    CHECK(calls_a[0].data.empty());

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
}

TEST_CASE("system_event_callback_remove stops further notifications for that callback only") {
    reset_calls();
    int context_a = 1;
    int context_b = 2;

    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);
    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_b, &context_b);

    CHECK_EQ(system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_a), ERROR_NONE);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);

    CHECK_EQ(calls_a.size(), 0);
    CHECK_EQ(calls_b.size(), 1);

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
}

TEST_CASE("system_event_callback_remove on an unregistered callback returns ERROR_NOT_FOUND and is a no-op") {
    reset_calls();
    int context_b = 2;
    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_b, &context_b);

    // listener_a was never added for this type, so removing it must not disturb listener_b.
    CHECK_EQ(system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_a), ERROR_NOT_FOUND);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    CHECK_EQ(calls_b.size(), 1);

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
}

TEST_CASE("system_event_callback_remove matches on (type, callback), not the callback alone") {
    reset_calls();
    int context_a = 1;

    // Same callback subscribed for two different event types.
    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);
    system_event_callback_add(KERNEL_EVENT_TIME_CHANGED, listener_a, &context_a);

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_a);

    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 0);

    system_event_emit(KERNEL_EVENT_TIME_CHANGED, nullptr, 0);
    CHECK_EQ(calls_a.size(), 1);

    system_event_callback_remove(KERNEL_EVENT_TIME_CHANGED, listener_a);
}

TEST_CASE("system_event_emit with no subscribers for that type returns ERROR_NONE") {
    CHECK_EQ(system_event_emit(KERNEL_EVENT_SERVICE_STOPPED, nullptr, 0), ERROR_NONE);
}

TEST_CASE("system_event_emit stamps the event with the current boot-relative time") {
    reset_calls();
    int context_a = 1;
    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_a, &context_a);

    auto before = static_cast<uint64_t>(get_micros_since_boot());
    system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
    auto after = static_cast<uint64_t>(get_micros_since_boot());

    REQUIRE_EQ(calls_a.size(), 1);
    CHECK_GE(calls_a[0].timestamp, before);
    CHECK_LE(calls_a[0].timestamp, after);

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_a);
}

static bool reentrant_add_triggered = false;

static void reentrant_listener(SystemEvent* event, void* context) {
    calls_a.push_back({ context, event->type, std::vector<uint8_t>(event->data, event->data + event->data_len), event->timestamp });
    if (!reentrant_add_triggered) {
        reentrant_add_triggered = true;
        // Subscribing from within a notification must not deadlock: emit() releases the
        // lock before invoking callbacks, so this only blocks briefly on the (already
        // unlocked) mutex.
        system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, listener_b, context);
        // Also exercise unsubscribe() and a nested emit() of a different type from within
        // a callback - all must complete without deadlocking.
        system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, reentrant_listener);
        system_event_emit(KERNEL_EVENT_TIME_CHANGED, nullptr, 0);
    }
}

TEST_CASE("system_event_emit is safe when a callback subscribes, unsubscribes and emits during notification") {
    reset_calls();
    reentrant_add_triggered = false;
    int context_a = 1;

    system_event_callback_add(KERNEL_EVENT_BOOT_COMPLETED, reentrant_listener, &context_a);
    system_event_callback_add(KERNEL_EVENT_TIME_CHANGED, listener_b, &context_a);

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

    system_event_callback_remove(KERNEL_EVENT_BOOT_COMPLETED, listener_b);
    system_event_callback_remove(KERNEL_EVENT_TIME_CHANGED, listener_b);
}

// gps.h-style poll subscription: system_event_subscribe()/_poll()/_unsubscribe().
//
// system_event_poll() only detects sequence increments that happened before it's called (same
// as gps_api_event_await()), so tests that need to observe an emit from another task block via
// task_event_group_wait() first - emitting before that wait started would race the notification
// the same way it would with any FreeRTOS task-notify consumer.

TEST_CASE("system_event_subscribe/_poll deliver the event payload by value") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_NETWORK_CONNECTED;
    CHECK_EQ(system_event_subscribe(&sub, &event_group), ERROR_NONE);

    NetworkConnectedEvent connected { .device = nullptr, .ipv4_addr = 0x0A000001, .gateway = 0x0A0000FE };
    auto* thread = thread_alloc_full(
        "system-event-emitter",
        4096,
        [](void* context) {
            delay_millis(20);
            auto* connected_ptr = static_cast<NetworkConnectedEvent*>(context);
            system_event_emit(KERNEL_EVENT_NETWORK_CONNECTED, connected_ptr, sizeof(*connected_ptr));
            return 0;
        },
        &connected,
        -1
    );
    CHECK_EQ(thread_start(thread), ERROR_NONE);

    CHECK_EQ(task_event_group_wait(&event_group, sub.bit, false, nullptr, pdMS_TO_TICKS(2000)), ERROR_NONE);
    CHECK_EQ(system_event_poll(&sub), ERROR_NONE);

    NetworkConnectedEvent received {};
    CHECK_EQ(system_event_get_data(&sub, reinterpret_cast<uint8_t*>(&received), sizeof(received)), ERROR_NONE);
    CHECK_EQ(received.ipv4_addr, connected.ipv4_addr);
    CHECK_EQ(received.gateway, connected.gateway);

    CHECK_EQ(thread_join(thread, pdMS_TO_TICKS(2000), pdMS_TO_TICKS(1)), ERROR_NONE);
    thread_free(thread);

    CHECK_EQ(system_event_unsubscribe(&sub), ERROR_NONE);
    CHECK_EQ(system_event_unsubscribe(&sub), ERROR_NOT_FOUND);

    task_event_group_destruct(&event_group);
}

TEST_CASE("system_event_poll returns a matching event that arrived before it was called") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_NETWORK_CONNECTED;
    CHECK_EQ(system_event_subscribe(&sub, &event_group), ERROR_NONE);

    // Same-thread emit, no background thread or wait needed: unlike the "detects a change after
    // it starts waiting" tests above, this is exactly the case system_event_poll() must handle -
    // sequence already moved ahead of consumed_sequence before poll() is even called.
    NetworkConnectedEvent connected { .device = nullptr, .ipv4_addr = 0x0A000001, .gateway = 0x0A0000FE };
    CHECK_EQ(system_event_emit(KERNEL_EVENT_NETWORK_CONNECTED, &connected, sizeof(connected)), ERROR_NONE);

    CHECK_EQ(system_event_poll(&sub), ERROR_NONE);

    NetworkConnectedEvent received {};
    CHECK_EQ(system_event_get_data(&sub, reinterpret_cast<uint8_t*>(&received), sizeof(received)), ERROR_NONE);
    CHECK_EQ(received.ipv4_addr, connected.ipv4_addr);
    CHECK_EQ(received.gateway, connected.gateway);

    // The pending event was consumed by the call above - a second poll() with no further emit
    // must time out rather than returning the same event again.
    CHECK_EQ(system_event_poll(&sub), ERROR_TIMEOUT);

    system_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("system_event_poll times out when no matching event has arrived") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_TIME_CHANGED;
    system_event_subscribe(&sub, &event_group);

    CHECK_EQ(system_event_poll(&sub), ERROR_TIMEOUT);

    system_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("system_event_emit does not notify a poll subscriber of a different type") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_BOOT_COMPLETED;
    system_event_subscribe(&sub, &event_group);

    system_event_emit(KERNEL_EVENT_TIME_CHANGED, nullptr, 0);
    CHECK_EQ(system_event_poll(&sub), ERROR_TIMEOUT);

    system_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("system_event_get_data reports ERROR_BUFFER_OVERFLOW and leaves the buffer untouched") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_NETWORK_DISCONNECTED;
    CHECK_EQ(system_event_subscribe(&sub, &event_group), ERROR_NONE);

    // Background emit + wait, same pattern as the payload-delivery test above - see the comment
    // near the top of the file.
    NetworkDisconnectedEvent disconnected { .device = nullptr };
    auto* thread = thread_alloc_full(
        "system-event-emitter",
        4096,
        [](void* context) {
            delay_millis(20);
            auto* disconnected_ptr = static_cast<NetworkDisconnectedEvent*>(context);
            system_event_emit(KERNEL_EVENT_NETWORK_DISCONNECTED, disconnected_ptr, sizeof(*disconnected_ptr));
            return 0;
        },
        &disconnected,
        -1
    );
    CHECK_EQ(thread_start(thread), ERROR_NONE);
    CHECK_EQ(task_event_group_wait(&event_group, sub.bit, false, nullptr, pdMS_TO_TICKS(2000)), ERROR_NONE);
    CHECK_EQ(system_event_poll(&sub), ERROR_NONE);
    CHECK_EQ(thread_join(thread, pdMS_TO_TICKS(2000), pdMS_TO_TICKS(1)), ERROR_NONE);
    thread_free(thread);

    uint8_t tiny[1] = { 0xAA };
    CHECK_EQ(system_event_get_data(&sub, tiny, sizeof(tiny)), ERROR_BUFFER_OVERFLOW);
    CHECK_EQ(tiny[0], 0xAA);

    uint8_t exact[sizeof(NetworkDisconnectedEvent)];
    CHECK_EQ(system_event_get_data(&sub, exact, sizeof(exact)), ERROR_NONE);

    system_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

TEST_CASE("system_event_get_data on a subscription with no payload copies nothing and succeeds") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_BOOT_COMPLETED;
    CHECK_EQ(system_event_subscribe(&sub, &event_group), ERROR_NONE);

    auto* thread = thread_alloc_full(
        "system-event-emitter",
        4096,
        [](void*) {
            delay_millis(20);
            system_event_emit(KERNEL_EVENT_BOOT_COMPLETED, nullptr, 0);
            return 0;
        },
        nullptr,
        -1
    );
    CHECK_EQ(thread_start(thread), ERROR_NONE);
    CHECK_EQ(task_event_group_wait(&event_group, sub.bit, false, nullptr, pdMS_TO_TICKS(2000)), ERROR_NONE);
    CHECK_EQ(system_event_poll(&sub), ERROR_NONE);
    CHECK_EQ(thread_join(thread, pdMS_TO_TICKS(2000), pdMS_TO_TICKS(1)), ERROR_NONE);
    thread_free(thread);

    uint8_t buffer[1] = { 0x42 };
    CHECK_EQ(system_event_get_data(&sub, buffer, 0), ERROR_NONE);
    CHECK_EQ(buffer[0], 0x42); // untouched - nothing to copy

    system_event_unsubscribe(&sub);
    task_event_group_destruct(&event_group);
}

// Regression coverage for system_event_unsubscribe()'s (now best-effort, not guaranteed - see
// its @warning in system_event.h) diagnostic signal, and for reusing a subscription node after
// unsubscribing it.

TEST_CASE("system_event_poll returns ERROR_INVALID_STATE after unsubscribe (diagnostic, best-effort)") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_SERVICE_STARTED;
    CHECK_EQ(system_event_subscribe(&sub, &event_group), ERROR_NONE);
    CHECK_EQ(system_event_unsubscribe(&sub), ERROR_NONE);

    // `sub` is still caller-owned storage after unsubscribe - polling it directly (rather than
    // still being blocked in task_event_group_wait() on it, which system_event_unsubscribe()'s
    // @warning now says not to do) is the one remaining diagnostic case `cancelled` covers.
    CHECK_EQ(system_event_poll(&sub), ERROR_INVALID_STATE);

    task_event_group_destruct(&event_group);
}

TEST_CASE("a subscription node can be re-subscribed after system_event_unsubscribe") {
    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    SystemEventSubscription sub {};
    sub.event.type = KERNEL_EVENT_SERVICE_STOPPED;

    CHECK_EQ(system_event_subscribe(&sub, &event_group), ERROR_NONE);
    CHECK_EQ(system_event_unsubscribe(&sub), ERROR_NONE);

    // Re-registering the same node (same storage, not a fresh SystemEventSubscription) must
    // work as if it were new - a fresh bit, and no leftover `cancelled` state from the
    // unsubscribe() above causing an immediate spurious ERROR_INVALID_STATE below.
    CHECK_EQ(system_event_subscribe(&sub, &event_group), ERROR_NONE);

    auto* thread = thread_alloc_full(
        "system-event-emitter",
        4096,
        [](void*) {
            delay_millis(20);
            system_event_emit(KERNEL_EVENT_SERVICE_STOPPED, nullptr, 0);
            return 0;
        },
        nullptr,
        -1
    );
    CHECK_EQ(thread_start(thread), ERROR_NONE);
    CHECK_EQ(task_event_group_wait(&event_group, sub.bit, false, nullptr, pdMS_TO_TICKS(2000)), ERROR_NONE);
    CHECK_EQ(system_event_poll(&sub), ERROR_NONE);
    CHECK_EQ(thread_join(thread, pdMS_TO_TICKS(2000), pdMS_TO_TICKS(1)), ERROR_NONE);
    thread_free(thread);

    CHECK_EQ(system_event_unsubscribe(&sub), ERROR_NONE);

    task_event_group_destruct(&event_group);
}
