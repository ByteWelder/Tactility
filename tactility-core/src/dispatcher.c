#include "dispatcher.h"

#include "tactility_core.h"

Dispatcher* tt_dispatcher_alloc(uint32_t message_count) {
    Dispatcher* dispatcher = malloc(sizeof(Dispatcher));
    *dispatcher = (Dispatcher) {
        .queue = tt_message_queue_alloc(message_count, sizeof(DispatcherMessage)),
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .buffer = {
            .callback = NULL,
            .context = NULL
        }
    };
    return dispatcher;
}

void tt_dispatcher_free(Dispatcher* dispatcher) {
    tt_mutex_acquire(dispatcher->mutex, TtWaitForever);
    tt_message_queue_reset(dispatcher->queue);
    tt_message_queue_free(dispatcher->queue);
    tt_mutex_release(dispatcher->mutex);
    tt_mutex_free(dispatcher->mutex);
    free(dispatcher);
}

void tt_dispatcher_dispatch(Dispatcher* dispatcher, Callback callback, void* context) {
    DispatcherMessage message = {
        .callback = callback,
        .context = context
    };
    tt_mutex_acquire(dispatcher->mutex, TtWaitForever);
    tt_message_queue_put(dispatcher->queue, &message, TtWaitForever);
    tt_mutex_release(dispatcher->mutex);
}

bool tt_dispatcher_consume(Dispatcher* dispatcher, uint32_t timeout_ticks) {
    tt_mutex_acquire(dispatcher->mutex, TtWaitForever);
    if (tt_message_queue_get(dispatcher->queue, &(dispatcher->buffer), timeout_ticks) == TtStatusOk) {
        DispatcherMessage* message = &(dispatcher->buffer);
        message->callback(message->context);
        tt_mutex_release(dispatcher->mutex);
        return true;
    } else {
        tt_mutex_release(dispatcher->mutex);
        return false;
    }
}
