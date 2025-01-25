#include <kernel/Kernel.h>
#include "Dispatcher.h"
#include "Check.h"

namespace tt {

#define TAG "dispatcher"
#define BACKPRESSURE_WARNING_COUNT ((EventBits_t)100)
#define WAIT_FLAG ((EventBits_t)1)

Dispatcher::~Dispatcher() {
    // Wait for Mutex usage
    mutex.lock();
    mutex.unlock();
}

void Dispatcher::dispatch(Function function, std::shared_ptr<void> context) {
    auto message = std::make_shared<DispatcherMessage>(function, std::move(context));
    // Mutate
    if (mutex.lock(1000 / portTICK_PERIOD_MS)) {
        queue.push(std::move(message));
        if (queue.size() == BACKPRESSURE_WARNING_COUNT) {
            TT_LOG_W(TAG, "Backpressure: You're not consuming fast enough (100 queued)");
        }
        tt_check(mutex.unlock());
        // Signal
        eventFlag.set(WAIT_FLAG);
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
    }
}

uint32_t Dispatcher::consume(TickType_t timeout) {
    // Wait for signal and clear
    TickType_t start_ticks = kernel::getTicks();
    if (eventFlag.wait(WAIT_FLAG, EventFlag::WaitAny, timeout)) {
        eventFlag.clear(WAIT_FLAG);
    } else {
        return 0;
    }

    TickType_t ticks_remaining = TT_MAX(timeout - (kernel::getTicks() - start_ticks), 0);

    TT_LOG_I(TAG, "Dispatcher continuing (%d ticks)", (int)ticks_remaining);

    // Mutate
    bool processing = true;
    uint32_t consumed = 0;
    do {
        if (mutex.lock(ticks_remaining / portTICK_PERIOD_MS)) {
            if (!queue.empty()) {
                auto item = queue.front();
                queue.pop();
                consumed++;
                processing = !queue.empty();
                // Don't keep lock as callback might be slow
                tt_check(mutex.unlock());
                item->function(item->context);
            } else {
                processing = false;
                tt_check(mutex.unlock());
            }

        } else {
            TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        }

    } while (processing);

    return consumed;
}

} // namespace
