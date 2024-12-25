#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef void* MessageQueueHandle;

MessageQueueHandle tt_message_queue_alloc(uint32_t capacity, uint32_t messageSize);
void tt_message_queue_free(MessageQueueHandle handle);
bool tt_message_queue_put(MessageQueueHandle handle, const void* message, uint32_t timeout);
bool tt_message_queue_get(MessageQueueHandle handle, void* message, uint32_t timeout);
uint32_t tt_message_queue_get_capacity(MessageQueueHandle handle);
uint32_t tt_message_queue_get_message_size(MessageQueueHandle handle);
uint32_t tt_message_queue_get_count(MessageQueueHandle handle);
bool tt_message_queue_reset(MessageQueueHandle handle);

#ifdef __cplusplus
}
#endif