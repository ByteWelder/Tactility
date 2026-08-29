// SPDX-License-Identifier: Apache-2.0
#include "doctest.h"

#include <app/io.h>
#include <app/loader.h>
#include <app/manager.h>
#include <app/scheduler.h>
#include <app/stream.h>

#include <service/manager.h>

#include <tactility/delay.h>

#include <atomic>
#include <cstring>
#include <vector>

extern ServiceManifest app_internal_loader_service_manifest;

namespace {

// See manager_test.cpp's own copy of this helper for why this checks the registry directly
// rather than a per-translation-unit static bool.
void ensure_memory_loader_registered() {
    if (service_manager_find_instance(APP_LOADER_MEMORY_SERVICE_ID) == nullptr) {
        service_manager_add(&app_internal_loader_service_manifest, /*auto_start=*/true);
    }
}

bool wait_for_state(AppInstanceId id, AppInstanceState target, uint32_t timeout_ms) {
    uint32_t waited = 0;
    while (waited < timeout_ms) {
        if (app_manager_get_state(id) == target) {
            return true;
        }
        delay_millis(10);
        waited += 10;
    }
    return app_manager_get_state(id) == target;
}

std::atomic<ssize_t> g_stdio_write_result { -2 };
std::atomic<ssize_t> g_stdio_read_result { -2 };

int32_t unbound_stdio_app_main(int, char*[]) {
    g_stdio_write_result.store(app_io_write(STDOUT_FILENO, "x", 1), std::memory_order_release);
    uint8_t buffer[1];
    g_stdio_read_result.store(app_io_read(STDIN_FILENO, buffer, sizeof(buffer)), std::memory_order_release);
    return 0;
}

int32_t stdout_writer_app_main(int, char*[]) {
    const char message[] = "hello";
    size_t sent = 0;
    while (sent < sizeof(message) - 1) {
        ssize_t written = app_io_write(STDOUT_FILENO, message + sent, sizeof(message) - 1 - sent);
        if (written < 0) {
            break;
        }
        sent += static_cast<size_t>(written);
    }
    return 0;
}

std::atomic<bool> g_blocked_writer_saw_error { false };
std::atomic<bool> g_blocked_writer_done { false };

int32_t blocked_writer_app_main(int, char*[]) {
    const char message[] = "0123456789"; // larger than the test's 4-byte stream capacity
    size_t sent = 0;
    while (sent < sizeof(message) - 1) {
        ssize_t written = app_io_write(STDOUT_FILENO, message + sent, sizeof(message) - 1 - sent);
        if (written < 0) {
            g_blocked_writer_saw_error.store(true, std::memory_order_release);
            break;
        }
        sent += static_cast<size_t>(written);
    }
    g_blocked_writer_done.store(true, std::memory_order_release);
    return 0;
}

} // namespace

TEST_CASE("an app's stdio fds default to the null device: write succeeds and discards, read reports EOF") {
    ensure_memory_loader_registered();
    g_stdio_write_result.store(-2, std::memory_order_relaxed);
    g_stdio_read_result.store(-2, std::memory_order_relaxed);

    AppManifest manifest { "test.io.unbound", "Unbound", APP_CATEGORY_USER, { APP_LOCATION_MEMORY, reinterpret_cast<void*>(unbound_stdio_app_main) } };
    REQUIRE_EQ(app_manager_add(&manifest), ERROR_NONE);

    AppInstanceId instance_id = 0;
    REQUIRE_EQ(app_manager_start("test.io.unbound", &instance_id), ERROR_NONE);
    REQUIRE(wait_for_state(instance_id, APP_INSTANCE_STATE_STOPPED, 1000));

    CHECK_EQ(g_stdio_write_result.load(std::memory_order_acquire), 1);
    CHECK_EQ(g_stdio_read_result.load(std::memory_order_acquire), 0);

    app_manager_remove("test.io.unbound");
}

TEST_CASE("app_manager_start_with_streams pipes a child's app_io_write() calls into a parent-owned AppStream, EOF at exit") {
    ensure_memory_loader_registered();

    AppManifest manifest { "test.io.writer", "Writer", APP_CATEGORY_USER, { APP_LOCATION_MEMORY, reinterpret_cast<void*>(stdout_writer_app_main) } };
    REQUIRE_EQ(app_manager_add(&manifest), ERROR_NONE);

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    uint8_t storage[64];
    AppStream child_stdout {};

    AppStreamBinding binding { STDOUT_FILENO, &child_stdout, storage, sizeof(storage), &event_group };
    AppInstanceId child_id = 0;
    REQUIRE_EQ(app_manager_start_with_streams("test.io.writer", &binding, 1, &child_id), ERROR_NONE);

    std::vector<uint8_t> received;
    while (app_stream_await(&child_stdout, APP_FILE_WAIT_READABLE, pdMS_TO_TICKS(1000)) == ERROR_NONE) {
        uint8_t chunk[16];
        size_t n = app_stream_read(&child_stdout, chunk, sizeof(chunk));
        if (n == 0) {
            break; // EOF
        }
        received.insert(received.end(), chunk, chunk + n);
    }

    REQUIRE_EQ(received.size(), 5u);
    CHECK_EQ(std::memcmp(received.data(), "hello", 5), 0);

    REQUIRE(wait_for_state(child_id, APP_INSTANCE_STATE_STOPPED, 1000));
    app_stream_unsubscribe(&child_stdout);
    task_event_group_destruct(&event_group);
    app_manager_remove("test.io.writer");
}

TEST_CASE("a write blocked on a full stream wakes with an error once the consumer closes it") {
    ensure_memory_loader_registered();
    g_blocked_writer_saw_error.store(false, std::memory_order_relaxed);
    g_blocked_writer_done.store(false, std::memory_order_relaxed);

    AppManifest manifest { "test.io.blocked", "Blocked", APP_CATEGORY_USER, { APP_LOCATION_MEMORY, reinterpret_cast<void*>(blocked_writer_app_main) } };
    REQUIRE_EQ(app_manager_add(&manifest), ERROR_NONE);

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    uint8_t storage[4]; // smaller than the 10 bytes blocked_writer_app_main sends
    AppStream child_stdout {};

    AppStreamBinding binding { STDOUT_FILENO, &child_stdout, storage, sizeof(storage), &event_group };
    AppInstanceId child_id = 0;
    REQUIRE_EQ(app_manager_start_with_streams("test.io.blocked", &binding, 1, &child_id), ERROR_NONE);

    // Never drained: the child fills the 4-byte buffer and blocks awaiting space for the rest.
    delay_millis(200);
    CHECK_FALSE(g_blocked_writer_done.load(std::memory_order_acquire));

    // Unblocks the writer without touching child_stdout's sync primitives, safe even while it
    // may still be blocked in app_stream_await() (unlike app_stream_unsubscribe()).
    app_stream_close(&child_stdout);

    REQUIRE(wait_for_state(child_id, APP_INSTANCE_STATE_STOPPED, 1000));
    CHECK(g_blocked_writer_saw_error.load(std::memory_order_acquire));

    // Only safe now that the child's task (the only other party that could be blocked on this
    // stream) has fully exited.
    app_stream_unsubscribe(&child_stdout);
    task_event_group_destruct(&event_group);
    app_manager_remove("test.io.blocked");
}

TEST_CASE("app_stream_unsubscribe is safe to call while a write is actively blocked") {
    ensure_memory_loader_registered();
    g_blocked_writer_saw_error.store(false, std::memory_order_relaxed);
    g_blocked_writer_done.store(false, std::memory_order_relaxed);

    AppManifest manifest { "test.io.unsub_race", "UnsubRace", APP_CATEGORY_USER, { APP_LOCATION_MEMORY, reinterpret_cast<void*>(blocked_writer_app_main) } };
    REQUIRE_EQ(app_manager_add(&manifest), ERROR_NONE);

    TaskEventGroup event_group {};
    task_event_group_construct(&event_group);

    uint8_t storage[4]; // smaller than the 10 bytes blocked_writer_app_main sends
    AppStream child_stdout {};

    AppStreamBinding binding { STDOUT_FILENO, &child_stdout, storage, sizeof(storage), &event_group };
    AppInstanceId child_id = 0;
    REQUIRE_EQ(app_manager_start_with_streams("test.io.unsub_race", &binding, 1, &child_id), ERROR_NONE);

    // Give the child time to fill the 4-byte buffer and block inside app_io_write(), already
    // dispatched through app_fd_table_get_and_retain() and currently waiting in
    // app_stream_await(). This is the exact state app_stream_unsubscribe() must be safe to run
    // against, with no prior app_stream_close() or wait for the child to stop first.
    delay_millis(200);
    REQUIRE_FALSE(g_blocked_writer_done.load(std::memory_order_acquire));

    // Regression: unsubscribing directly here used to be able to destruct
    // stream->internal.mutex while the blocked write above was still executing against it.
    REQUIRE_EQ(app_stream_unsubscribe(&child_stdout), ERROR_NONE);

    REQUIRE(wait_for_state(child_id, APP_INSTANCE_STATE_STOPPED, 1000));
    CHECK(g_blocked_writer_saw_error.load(std::memory_order_acquire));

    task_event_group_destruct(&event_group);
    app_manager_remove("test.io.unsub_race");
}
