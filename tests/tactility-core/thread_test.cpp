#include "doctest.h"
#include "thread.h"

#include "FreeRTOS.h"
#include "task.h"

static int interruptable_thread(TT_UNUSED void* parameter) {
    bool* interrupted = (bool*)parameter;
    while (!*interrupted) {
        vTaskDelay(5);
    }
    return 0;
}

static bool immedate_return_thread_called = false;
static int immediate_return_thread(TT_UNUSED void* parameter) {
    immedate_return_thread_called = true;
    return 0;
}

static int thread_with_return_code(TT_UNUSED void* parameter) {
    int* code = (int*)parameter;
    return *code;
}

TEST_CASE("when a thread is started then its callback should be called") {
    Thread* thread = tt_thread_alloc_ex(
        "immediate return task",
        4096,
        &immediate_return_thread,
        NULL
    );
    CHECK(!immedate_return_thread_called);
    tt_thread_start(thread);
    tt_thread_join(thread);
    tt_thread_free(thread);
    CHECK(immedate_return_thread_called);
}

TEST_CASE("a thread can be started and stopped") {
    bool interrupted = false;
    Thread* thread = tt_thread_alloc_ex(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK(thread);
    tt_thread_start(thread);
    interrupted = true;
    tt_thread_join(thread);
    tt_thread_free(thread);
}

TEST_CASE("thread id should only be set at when thread is started") {
    bool interrupted = false;
    Thread* thread = tt_thread_alloc_ex(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK(tt_thread_get_id(thread) == NULL);
    tt_thread_start(thread);
    CHECK(tt_thread_get_id(thread) != NULL);
    interrupted = true;
    tt_thread_join(thread);
    CHECK(tt_thread_get_id(thread) == NULL);
    tt_thread_free(thread);
}

TEST_CASE("thread state should be correct") {
    bool interrupted = false;
    Thread* thread = tt_thread_alloc_ex(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK_EQ(tt_thread_get_state(thread), ThreadStateStopped);
    tt_thread_start(thread);
    ThreadState state = tt_thread_get_state(thread);
    CHECK((state == ThreadStateStarting || state == ThreadStateRunning));
    interrupted = true;
    tt_thread_join(thread);
    CHECK_EQ(tt_thread_get_state(thread), ThreadStateStopped);
    tt_thread_free(thread);
}

TEST_CASE("thread id should only be set at when thread is started") {
    int code = 123;
    Thread* thread = tt_thread_alloc_ex(
        "return code",
        4096,
        &thread_with_return_code,
        &code
    );
    tt_thread_start(thread);
    tt_thread_join(thread);
    CHECK_EQ(tt_thread_get_return_code(thread), code);
    tt_thread_free(thread);
}
