#include "doctest.h"
#include "TactilityCore.h"
#include "Thread.h"

using namespace tt;

static int interruptable_thread(void* parameter) {
    bool* interrupted = (bool*)parameter;
    while (!*interrupted) {
        kernel::delayMillis(5);
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
    auto* thread = new Thread(
        "immediate return task",
        4096,
        &immediate_return_thread,
        &has_called
    );
    CHECK(!has_called);
    thread->start();
    thread->join();
    delete thread;
    CHECK(has_called);
}

TEST_CASE("a thread can be started and stopped") {
    bool interrupted = false;
    auto* thread = new Thread(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK(thread);
    thread->start();
    interrupted = true;
    thread->join();
    delete thread;
}

TEST_CASE("thread id should only be set at when thread is started") {
    bool interrupted = false;
    auto* thread = new Thread(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK_EQ(thread->getId(), nullptr);
    thread->start();
    CHECK_NE(thread->getId(), nullptr);
    interrupted = true;
    thread->join();
    CHECK_EQ(thread->getId(), nullptr);
    delete thread;
}

TEST_CASE("thread state should be correct") {
    bool interrupted = false;
    auto* thread = new Thread(
        "interruptable thread",
        4096,
        &interruptable_thread,
        &interrupted
    );
    CHECK_EQ(thread->getState(), Thread::StateStopped);
    thread->start();
    Thread::State state = thread->getState();
    CHECK((state == Thread::StateStarting || state == Thread::StateRunning));
    interrupted = true;
    thread->join();
    CHECK_EQ(thread->getState(), Thread::StateStopped);
    delete thread;
}

TEST_CASE("thread id should only be set at when thread is started") {
    int code = 123;
    auto* thread = new Thread(
        "return code",
        4096,
        &thread_with_return_code,
        &code
    );
    thread->start();
    thread->join();
    CHECK_EQ(thread->getReturnCode(), code);
    delete thread;
}
