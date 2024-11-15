#include "dispatcher.h"

#include "tactility_core.h"

typedef struct {
    Callback callback;
    void* context;
} DispatcherMessage;

typedef struct {
    MessageQueue* queue;
    Mutex* mutex;
    DispatcherMessage buffer; // Buffer for consuming a message
} DispatcherData;

Dispatcher* tt_dispatcher_alloc(uint32_t message_count) {
    auto* data = static_cast<DispatcherData*>(malloc(sizeof(DispatcherData)));
    *data = (DispatcherData) {
        .queue = tt_message_queue_alloc(message_count, sizeof(DispatcherMessage)),
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .buffer = {
            .callback = nullptr,
            .context = nullptr
        }
    };
    return data;
}

void tt_dispatcher_free(Dispatcher* dispatcher) {
    auto* data = static_cast<DispatcherData*>(dispatcher);
    tt_mutex_acquire(data->mutex, TtWaitForever);
    tt_message_queue_reset(data->queue);
    tt_message_queue_free(data->queue);
    tt_mutex_release(data->mutex);
    tt_mutex_free(data->mutex);
    free(data);
}

void tt_dispatcher_dispatch(Dispatcher* dispatcher, Callback callback, void* context) {
    auto* data = (DispatcherData*)dispatcher;
    DispatcherMessage message = {
        .callback = callback,
        .context = context
    };
    tt_mutex_acquire(data->mutex, TtWaitForever);
    tt_message_queue_put(data->queue, &message, TtWaitForever);
    tt_mutex_release(data->mutex);
}

bool tt_dispatcher_consume(Dispatcher* dispatcher, uint32_t timeout_ticks) {
    auto* data = static_cast<DispatcherData*>(dispatcher);
    tt_mutex_acquire(data->mutex, TtWaitForever);
    if (tt_message_queue_get(data->queue, &(data->buffer), timeout_ticks) == TtStatusOk) {
        DispatcherMessage* message = &(data->buffer);
        message->callback(message->context);
        tt_mutex_release(data->mutex);
        return true;
    } else {
        tt_mutex_release(data->mutex);
        return false;
    }
}
