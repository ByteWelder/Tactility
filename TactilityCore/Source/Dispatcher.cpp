#include <kernel/Kernel.h>
#include "Dispatcher.h"
#include "Check.h"

namespace tt {

#define TAG "Dispatcher"
#define BACKPRESSURE_WARNING_COUNT 100

Dispatcher::Dispatcher() :
    mutex(Mutex::TypeNormal)
{}

Dispatcher::~Dispatcher() {
    // Wait for Mutex usage
    mutex.acquire(TtWaitForever);
    mutex.release();
}

void Dispatcher::dispatch(Callback callback, std::shared_ptr<void> context) {
    auto message = std::make_shared<DispatcherMessage>(callback, std::move(context));
    // Mutate
    if (mutex.lock(1000 / portTICK_PERIOD_MS)) {
        queue.push(std::move(message));
        TT_LOG_I(TAG, "dispatch");
        if (queue.size() == BACKPRESSURE_WARNING_COUNT) {
            TT_LOG_W(TAG, "Backpressure: You're not consuming fast enough (100 queued)");
        }
        mutex.unlock();
        // Signal
        eventFlag.set(1);
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
    }
}

uint32_t Dispatcher::consume(uint32_t timeout_ticks) {
    // Wait for signal and clear
    TickType_t start_ticks = kernel::getTicks();
    if (eventFlag.wait(1, TtFlagWaitAny, timeout_ticks) == TtStatusErrorTimeout) {
        return 0;
    }

    TickType_t ticks_remaining = TT_MAX(timeout_ticks - (kernel::getTicks() - start_ticks), 0);

    eventFlag.clear(1);

    TT_LOG_I(TAG, "Dispatcher continuing");

    // Mutate
    bool processing = true;
    uint32_t consumed = 0;
    do {
        if (mutex.lock(ticks_remaining / portTICK_PERIOD_MS)) {
            if (!queue.empty()) {
                TT_LOG_I(TAG, "Dispatcher popping from queue");
                auto item = queue.front();
                queue.pop();
                consumed++;
                processing = !queue.empty();
                // Don't keep lock as callback might be slow
                tt_check(mutex.unlock());

                item->callback(item->context);
            }
        } else {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        }

    } while (processing);

    return consumed;
}

} // namespace
