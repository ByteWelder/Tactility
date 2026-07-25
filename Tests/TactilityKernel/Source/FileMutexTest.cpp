#include "doctest.h"
#include <tactility/filesystem/file_mutex.h>

namespace {

int lock_calls = 0;
int unlock_calls = 0;
int try_lock_calls = 0;
bool try_lock_result = true;
uint32_t try_lock_timeout_seen = 0;

void mock_lock() { lock_calls++; }
void mock_unlock() { unlock_calls++; }
bool mock_try_lock(uint32_t timeout) {
    try_lock_calls++;
    try_lock_timeout_seen = timeout;
    return try_lock_result;
}

int lock_a_calls = 0;
int lock_b_calls = 0;
void mock_lock_a() { lock_a_calls++; }
void mock_lock_b() { lock_b_calls++; }

void reset_mocks() {
    lock_calls = 0;
    unlock_calls = 0;
    try_lock_calls = 0;
    try_lock_result = true;
    try_lock_timeout_seen = 0;
    lock_a_calls = 0;
    lock_b_calls = 0;
}

} // namespace

TEST_CASE("file_mutex_get with zero registrations returns a no-op mutex") {
    FileMutex mutex;
    file_mutex_get(&mutex, "/nowhere/file.txt");

    CHECK_EQ(mutex.lock, nullptr);
    CHECK_EQ(mutex.try_lock, nullptr);
    CHECK_EQ(mutex.unlock, nullptr);

    // Calling through a no-op mutex must be safe, and try_lock must report success.
    file_mutex_lock(&mutex);
    CHECK_EQ(file_mutex_try_lock(&mutex, 123), true);
    file_mutex_unlock(&mutex);
}

TEST_CASE("file_mutex_register/get with a single registration") {
    reset_mocks();
    FileMutex registered = { .lock = mock_lock, .try_lock = mock_try_lock, .unlock = mock_unlock };
    file_mutex_register(&registered, "/mock1");

    FileMutex mutex;

    // Exact mount path match.
    file_mutex_get(&mutex, "/mock1");
    CHECK_EQ(mutex.lock, mock_lock);
    CHECK_EQ(mutex.try_lock, mock_try_lock);
    CHECK_EQ(mutex.unlock, mock_unlock);

    // Descendant path match.
    file_mutex_get(&mutex, "/mock1/nested/file.txt");
    CHECK_EQ(mutex.lock, mock_lock);

    // Unrelated path falls back to no-op.
    FileMutex unrelated;
    file_mutex_get(&unrelated, "/other/file.txt");
    CHECK_EQ(unrelated.lock, nullptr);

    // Prefix-but-not-descendant path (e.g. "/mock1x") must not match "/mock1".
    FileMutex prefix_only;
    file_mutex_get(&prefix_only, "/mock1x/file.txt");
    CHECK_EQ(prefix_only.lock, nullptr);

    // Exercise the resolved callbacks.
    file_mutex_get(&mutex, "/mock1");
    file_mutex_lock(&mutex);
    CHECK_EQ(lock_calls, 1);
    CHECK_EQ(file_mutex_try_lock(&mutex, 42), true);
    CHECK_EQ(try_lock_calls, 1);
    CHECK_EQ(try_lock_timeout_seen, 42);
    file_mutex_unlock(&mutex);
    CHECK_EQ(unlock_calls, 1);

    // Re-registering the same path is a no-op: original callbacks remain in place.
    FileMutex replacement = { .lock = nullptr, .try_lock = nullptr, .unlock = nullptr };
    file_mutex_register(&replacement, "/mock1");
    file_mutex_get(&mutex, "/mock1");
    CHECK_EQ(mutex.lock, mock_lock);
}

TEST_CASE("file_mutex_register/get with two registrations resolves to the matching path") {
    reset_mocks();

    FileMutex mutex_a = { .lock = mock_lock_a, .try_lock = nullptr, .unlock = nullptr };
    FileMutex mutex_b = { .lock = mock_lock_b, .try_lock = nullptr, .unlock = nullptr };

    file_mutex_register(&mutex_a, "/mock2a");
    file_mutex_register(&mutex_b, "/mock2b");

    FileMutex resolved;

    file_mutex_get(&resolved, "/mock2a/file.txt");
    CHECK_EQ(resolved.lock, mock_lock_a);

    file_mutex_get(&resolved, "/mock2b/file.txt");
    CHECK_EQ(resolved.lock, mock_lock_b);

    // Path matching neither registration falls back to no-op.
    file_mutex_get(&resolved, "/mock2c/file.txt");
    CHECK_EQ(resolved.lock, nullptr);

    // Registration order matters: the first matching entry wins, not the longest
    // prefix. A mount nested under an earlier one is shadowed by it.
    FileMutex mutex_nested = { .lock = nullptr, .try_lock = nullptr, .unlock = nullptr };
    file_mutex_register(&mutex_nested, "/mock2a/nested");
    file_mutex_get(&resolved, "/mock2a/nested/file.txt");
    CHECK_EQ(resolved.lock, mock_lock_a); // still /mock2a, registered first
}
