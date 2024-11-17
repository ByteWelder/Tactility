#include "doctest.h"
#include "MessageQueue.h"

TEST_CASE("message queue capacity should be correct") {
    MessageQueue queue(10, 1);

    uint32_t capacity = queue.getCapacity();
    CHECK_EQ(capacity, 10);
}

TEST_CASE("message queue initial count should be 0") {
    MessageQueue queue(10, 1);

    uint32_t count = queue.getCount();
    CHECK_EQ(count, 0);
}

TEST_CASE("message queue size should be correct") {
    MessageQueue queue(1, 123);

    uint32_t count = queue.getCount();
    CHECK_EQ(count, 123);
}

TEST_CASE("message queue count should increase when message is added") {
    MessageQueue queue(10, sizeof(uint32_t));

    uint32_t message = 123;
    queue.put(&message, 100);
    uint32_t count = queue.getCount();
    CHECK_EQ(count, 1);
}

TEST_CASE("message queue count should be 0 when message is added and queue is reset") {
    MessageQueue queue(10, sizeof(uint32_t));

    uint32_t message = 123;

    queue.put(&message, 100);
    queue.reset();
    uint32_t count = queue.getCount();
    CHECK_EQ(count, 0);
}

TEST_CASE("message queue consumption should work") {
    MessageQueue queue(10, sizeof(uint32_t));

    uint32_t out_message = 123;
    queue.put(&out_message, 100);

    uint32_t in_message = 0;
    queue.get(&in_message, 100);
    CHECK_EQ(in_message, 123);
}

TEST_CASE("message queue count should decrease when message is consumed") {
    MessageQueue queue(10, sizeof(uint32_t));

    uint32_t out_message = 123;
    queue.put(&out_message, 100);

    uint32_t in_message = 0;
    queue.get(&in_message, 100);
    uint32_t count = queue.getCount();
    CHECK_EQ(count, 0);
}
