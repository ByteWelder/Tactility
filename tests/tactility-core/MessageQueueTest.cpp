#include "doctest.h"
#include "message_queue.h"

TEST_CASE("message queue capacity should be correct") {
    MessageQueue* queue = tt_message_queue_alloc(10, 1);

    uint32_t capacity = tt_message_queue_get_capacity(queue);
    CHECK_EQ(capacity, 10);

    tt_message_queue_free(queue);
}

TEST_CASE("message queue initial count should be 0") {
    MessageQueue* queue = tt_message_queue_alloc(10, 1);

    uint32_t count = tt_message_queue_get_count(queue);
    CHECK_EQ(count, 0);

    tt_message_queue_free(queue);
}

TEST_CASE("message queue size should be correct") {
    MessageQueue* queue = tt_message_queue_alloc(1, 123);

    uint32_t size = tt_message_queue_get_message_size(queue);
    CHECK_EQ(size, 123);

    tt_message_queue_free(queue);
}

TEST_CASE("message queue count should increase when message is added") {
    MessageQueue* queue = tt_message_queue_alloc(10, sizeof(uint32_t));

    uint32_t message = 123;

    tt_message_queue_put(queue, &message, 100);
    uint32_t count = tt_message_queue_get_count(queue);
    CHECK_EQ(count, 1);

    tt_message_queue_free(queue);
}

TEST_CASE("message queue count should be 0 when message is added and queue is reset") {
    MessageQueue* queue = tt_message_queue_alloc(10, sizeof(uint32_t));

    uint32_t message = 123;

    tt_message_queue_put(queue, &message, 100);
    tt_message_queue_reset(queue);
    uint32_t count = tt_message_queue_get_count(queue);
    CHECK_EQ(count, 0);

    tt_message_queue_free(queue);
}

TEST_CASE("message queue consumption should work") {
    MessageQueue* queue = tt_message_queue_alloc(10, sizeof(uint32_t));

    uint32_t out_message = 123;
    tt_message_queue_put(queue, &out_message, 100);

    uint32_t in_message = 0;
    tt_message_queue_get(queue, &in_message, 100);
    CHECK_EQ(in_message, 123);

    tt_message_queue_free(queue);
}

TEST_CASE("message queue count should decrease when message is consumed") {
    MessageQueue* queue = tt_message_queue_alloc(10, sizeof(uint32_t));

    uint32_t out_message = 123;
    tt_message_queue_put(queue, &out_message, 100);

    uint32_t in_message = 0;
    tt_message_queue_get(queue, &in_message, 100);
    uint32_t count = tt_message_queue_get_count(queue);
    CHECK_EQ(count, 0);

    tt_message_queue_free(queue);
}
