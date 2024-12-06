#include "Dispatcher.h"
#include "Check.h"

namespace tt {

#define TAG "Dispatcher"
#define BACKPRESSURE_WARNING_COUNT 100

Dispatcher::Dispatcher() :
    mutex(MutexTypeNormal)
{}

Dispatcher::~Dispatcher() {
    // Wait for Mutex usage
    mutex.acquire(TtWaitForever);
    mutex.release();
}

void Dispatcher::dispatch(Callback callback, std::shared_ptr<void> context) {
    auto message = std::make_shared<DispatcherMessage>(callback, std::move(context));
    // Mutate
    mutex.acquire(TtWaitForever);
    queue.push(std::move(message));
    if (queue.size() == BACKPRESSURE_WARNING_COUNT) {
        TT_LOG_W(TAG, "Backpressure: You're not consuming fast enough (100 queued)");
    }
    mutex.release();
    // Signal
    eventFlag.set(1);
}

uint32_t Dispatcher::consume(uint32_t timeout_ticks) {
    // Wait for signal and clear
    eventFlag.wait(1, TtFlagWaitAny, timeout_ticks);
    eventFlag.clear(1);

    // Mutate
    if (mutex.acquire(1 / portTICK_PERIOD_MS) == TtStatusOk) {
        auto item = queue.front();
        queue.pop();
        // Don't keep lock as callback might be slow
        tt_check(mutex.release() == TtStatusOk);

        item->callback(item->context);
    }

    return true;
}

} // namespace
