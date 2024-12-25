#include "tt_message_queue.h"
#include <MessageQueue.h>

#define HANDLE_TO_MESSAGE_QUEUE(handle) ((tt::MessageQueue*)(handle))

extern "C" {

MessageQueueHandle tt_message_queue_alloc(uint32_t capacity, uint32_t messageSize) {
    return new tt::MessageQueue(capacity, messageSize);
}

void tt_message_queue_free(MessageQueueHandle handle) {
    delete HANDLE_TO_MESSAGE_QUEUE(handle);
}

bool tt_message_queue_put(MessageQueueHandle handle, const void* message, uint32_t timeout) {
    return HANDLE_TO_MESSAGE_QUEUE(handle)->put(message, timeout);
}

bool tt_message_queue_get(MessageQueueHandle handle, void* message, uint32_t timeout) {
    return HANDLE_TO_MESSAGE_QUEUE(handle)->get(message, timeout);
}

uint32_t tt_message_queue_get_capacity(MessageQueueHandle handle) {
    return HANDLE_TO_MESSAGE_QUEUE(handle)->getCapacity();
}

uint32_t tt_message_queue_get_message_size(MessageQueueHandle handle) {
    return HANDLE_TO_MESSAGE_QUEUE(handle)->getMessageSize();
}

uint32_t tt_message_queue_get_count(MessageQueueHandle handle) {
    return HANDLE_TO_MESSAGE_QUEUE(handle)->getCount();
}

bool tt_message_queue_reset(MessageQueueHandle handle) {
    return HANDLE_TO_MESSAGE_QUEUE(handle)->reset();
}

}
