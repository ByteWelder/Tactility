#include "doctest.h"
#include "TactilityCore.h"
#include "Dispatcher.h"

using namespace tt;

static uint32_t counter = 0;
static const uint32_t value_chacker_expected = 123;

void increment_callback(TT_UNUSED std::shared_ptr<void> context) {
    counter++;
}

void value_checker(std::shared_ptr<void> context) {
    auto value = std::static_pointer_cast<uint32_t>(context);
    if (*value != value_chacker_expected) {
        tt_crash("Test error");
    }
}

TEST_CASE("dispatcher should not call callback if consume isn't called") {
    counter = 0;
    Dispatcher dispatcher;

    auto context = std::make_shared<uint32_t>();
    dispatcher.dispatch(&increment_callback, std::move(context));
    kernel::delayTicks(10);

    CHECK_EQ(counter, 0);
}

TEST_CASE("dispatcher should be able to dealloc when message is not consumed") {
    auto* dispatcher = new Dispatcher();
    auto context = std::make_shared<uint32_t>();
    dispatcher->dispatch(increment_callback, std::move(context));
    delete dispatcher;
}

TEST_CASE("dispatcher should call callback when consume is called") {
    counter = 0;
    Dispatcher dispatcher;

    auto context = std::make_shared<uint32_t>();
    dispatcher.dispatch(increment_callback, std::move(context));
    dispatcher.consume(100);
    CHECK_EQ(counter, 1);
}

TEST_CASE("message should be passed on correctly") {
    Dispatcher dispatcher;

    auto context = std::make_shared<uint32_t>(value_chacker_expected);
    dispatcher.dispatch(value_checker, std::move(context));
    dispatcher.consume(100);
}
