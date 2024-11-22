#include "doctest.h"
#include "tactility_core.h"
#include "Dispatcher.h"

using namespace tt;

void increment_callback(void* context) {
    auto* counter = (uint32_t*)context;
    (*counter)++;
}

TEST_CASE("dispatcher should not call callback if consume isn't called") {
    Dispatcher dispatcher;

    uint32_t counter = 0;
    dispatcher.dispatch(&increment_callback, &counter);
    delay_tick(10);

    CHECK_EQ(counter, 0);
}

TEST_CASE("dispatcher should be able to dealloc when message is not consumed") {
    auto* dispatcher = new Dispatcher();
    uint32_t counter = 0;
    dispatcher->dispatch(increment_callback, &counter);
    delete dispatcher;
}

TEST_CASE("dispatcher should call callback when consume is called") {
    Dispatcher dispatcher;

    uint32_t counter = 0;
    dispatcher.dispatch(increment_callback, &counter);
    dispatcher.consume(100);
    CHECK_EQ(counter, 1);
}
