#include "doctest.h"
#include "tactility_core.h"
#include "dispatcher.h"

void increment_callback(void* context) {
    auto* counter = (uint32_t*)context;
    (*counter)++;
}

TEST_CASE("dispatcher should not call callback if consume isn't called") {
    auto* dispatcher = tt_dispatcher_alloc(10);

    uint32_t counter = 0;
    tt_dispatcher_dispatch(dispatcher, &increment_callback, &counter);
    tt_delay_tick(10);
    CHECK_EQ(counter, 0);

    tt_dispatcher_free(dispatcher);
}


TEST_CASE("dispatcher should be able to dealloc when message is not consumed") {
    auto* dispatcher = tt_dispatcher_alloc(10);
    uint32_t counter = 0;
    tt_dispatcher_dispatch(dispatcher, increment_callback, &counter);
    tt_dispatcher_free(dispatcher);
}

TEST_CASE("dispatcher should call callback when consume is called") {
    auto* dispatcher = tt_dispatcher_alloc(10);

    uint32_t counter = 0;
    tt_dispatcher_dispatch(dispatcher, increment_callback, &counter);
    tt_dispatcher_consume(dispatcher, 100);
    CHECK_EQ(counter, 1);

    tt_dispatcher_free(dispatcher);
}
