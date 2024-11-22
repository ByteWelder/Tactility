#include "doctest.h"
#include "TactilityCore.h"
#include "Thread.h"

using namespace tt;

static int interruptable_thread(void* parameter) {
    bool* interrupted = (bool*)parameter;
    while (!*interrupted) {
        delay_ms(5);
    }
    return 0;
}

static int immediate_return_thread(void* parameter) {
    bool* has_called = (bool*)parameter;
    *has_called = true;
    return 0;
}

static int thread_with_return_code(void* parameter) {
    int* code = (int*)parameter;
    return *code;
}

TEST_CASE("when a thread is started then its callback should be called") {
    bool has_called = false;
    auto* thread = thread_alloc_ex(
        "immediate return task",
        4096,
        &immediate_return_thread,
        &has_called
    );
    CHECK(!has_called);
    thread_start(thread);
    thread_join(thread);
    thread_free(thread);
    CHECK(has_called);
}

TEST_CASE("a thread can be started and stopped") {
    bool interrupted = false;
    auto* thread = thread_alloc_ex(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK(thread);
    thread_start(thread);
    interrupted = true;
    thread_join(thread);
    thread_free(thread);
}

TEST_CASE("thread id should only be set at when thread is started") {
    bool interrupted = false;
    auto* thread = thread_alloc_ex(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK_EQ(thread_get_id(thread), nullptr);
    thread_start(thread);
    CHECK_NE(thread_get_id(thread), nullptr);
    interrupted = true;
    thread_join(thread);
    CHECK_EQ(thread_get_id(thread), nullptr);
    thread_free(thread);
}

TEST_CASE("thread state should be correct") {
    bool interrupted = false;
    auto* thread = thread_alloc_ex(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK_EQ(thread_get_state(thread), ThreadStateStopped);
    thread_start(thread);
    State state = thread_get_state(thread);
    CHECK((state == ThreadStateStarting || state == ThreadStateRunning));
    interrupted = true;
    thread_join(thread);
    CHECK_EQ(thread_get_state(thread), ThreadStateStopped);
    thread_free(thread);
}

TEST_CASE("thread id should only be set at when thread is started") {
    int code = 123;
    auto* thread = thread_alloc_ex(
        "return code",
        4096,
        &thread_with_return_code,
        &code
    );
    thread_start(thread);
    thread_join(thread);
    CHECK_EQ(thread_get_return_code(thread), code);
    thread_free(thread);
}
