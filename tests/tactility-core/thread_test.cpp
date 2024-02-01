#include "doctest.h"
#include "thread.h"

static int thread_callback(TT_UNUSED void* parameter) {
    return 0;
}

TEST_CASE("a thread can be started and stopped") {
    Thread* thread = tt_thread_alloc_ex(
        "test_thread",
        4096,
        &thread_callback,
        NULL
    );
    CHECK(thread != NULL);
    tt_thread_start(thread);
    tt_thread_join(thread);
    tt_thread_free(thread);
}
