#include "Dispatcher.h"

namespace tt {

Dispatcher::Dispatcher(size_t queueLimit) :
    queue(queueLimit, sizeof(DispatcherMessage)),
    mutex(MutexTypeNormal),
    buffer({ .callback = nullptr, .context = nullptr }) { }

Dispatcher::~Dispatcher() {
    queue.reset();
    // Wait for Mutex usage
    mutex.acquire(TtWaitForever);
    mutex.release();
}

void Dispatcher::dispatch(Callback callback, void* context) {
    DispatcherMessage message = {
        .callback = callback,
        .context = context
    };
    mutex.acquire(TtWaitForever);
    queue.put(&message, TtWaitForever);
    mutex.release();
}

bool Dispatcher::consume(uint32_t timeout_ticks) {
    mutex.acquire(TtWaitForever);
    if (queue.get(&buffer, timeout_ticks) == TtStatusOk) {
        buffer.callback(buffer.context);
        mutex.release();
        return true;
    } else {
        mutex.release();
        return false;
    }
}

} // namespace
