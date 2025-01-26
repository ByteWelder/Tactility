#include <kernel/Kernel.h>
#include "Dispatcher.h"
#include "Check.h"

namespace tt {

#define TAG "dispatcher"
#define BACKPRESSURE_WARNING_COUNT ((EventBits_t)100)
#define WAIT_FLAG ((EventBits_t)1U)

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
    // Wait for signal
    uint32_t result = eventFlag.wait(WAIT_FLAG, EventFlag::WaitAny, timeout);
    if (result & EventFlag::Error) {
        return 0;
    }

    eventFlag.clear(WAIT_FLAG);

    // Mutate
    bool processing = true;
    uint32_t consumed = 0;
    do {
        if (mutex.lock(10)) {
            if (!queue.empty()) {
                auto item = queue.front();
                queue.pop();
                consumed++;
                processing = !queue.empty();
                // Don't keep lock as callback might be slow
                mutex.unlock();
                item->function(item->context);
            } else {
                processing = false;
                mutex.unlock();
            }
        } else {
            TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        }

    } while (processing);

    return consumed;
}

} // namespace
