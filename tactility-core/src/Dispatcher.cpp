#include "Dispatcher.h"

Dispatcher::Dispatcher(size_t queueLimit) :
    queue(queueLimit, sizeof(DispatcherMessage)) {

    mutex = tt_mutex_alloc(MutexTypeNormal);
    buffer = {
        .callback = nullptr,
        .context = nullptr
    };
}

Dispatcher::~Dispatcher() {
    queue.reset();
    tt_mutex_acquire(mutex, TtWaitForever);
    tt_mutex_release(mutex);
    tt_mutex_free(mutex);
}

void Dispatcher::dispatch(Callback callback, void* context) {
    DispatcherMessage message = {
        .callback = callback,
        .context = context
    };
    tt_mutex_acquire(mutex, TtWaitForever);
    queue.put(&message, TtWaitForever);
    tt_mutex_release(mutex);
}

bool Dispatcher::consume(uint32_t timeout_ticks) {
    tt_mutex_acquire(mutex, TtWaitForever);
    if (queue.get(&buffer, timeout_ticks) == TtStatusOk) {
        DispatcherMessage* message = &(buffer);
        message->callback(message->context);
        tt_mutex_release(mutex);
        return true;
    } else {
        tt_mutex_release(mutex);
        return false;
    }
}
