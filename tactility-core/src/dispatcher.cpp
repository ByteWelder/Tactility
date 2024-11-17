#include "dispatcher.h"

Dispatcher::Dispatcher(size_t queueLimit) {
    queue = tt_message_queue_alloc(queueLimit, sizeof(DispatcherMessage));
    mutex = tt_mutex_alloc(MutexTypeNormal);
    buffer = {
        .callback = nullptr,
        .context = nullptr
    };
}

Dispatcher::~Dispatcher() {
    tt_mutex_acquire(mutex, TtWaitForever);
    tt_message_queue_reset(queue);
    tt_message_queue_free(queue);
    tt_mutex_release(mutex);
    tt_mutex_free(mutex);
}

void Dispatcher::dispatch(Callback callback, void* context) {
    DispatcherMessage message = {
        .callback = callback,
        .context = context
    };
    tt_mutex_acquire(mutex, TtWaitForever);
    tt_message_queue_put(queue, &message, TtWaitForever);
    tt_mutex_release(mutex);
}

bool Dispatcher::consume(uint32_t timeout_ticks) {
    tt_mutex_acquire(mutex, TtWaitForever);
    if (tt_message_queue_get(queue, &(buffer), timeout_ticks) == TtStatusOk) {
        DispatcherMessage* message = &(buffer);
        message->callback(message->context);
        tt_mutex_release(mutex);
        return true;
    } else {
        tt_mutex_release(mutex);
        return false;
    }
}
