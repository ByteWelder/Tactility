#include "Tactility/Dispatcher.h"

#include "Tactility/Check.h"
#include "Tactility/kernel/Kernel.h"

namespace tt {

#define TAG "dispatcher"
#define BACKPRESSURE_WARNING_COUNT ((EventBits_t)100)
#define WAIT_FLAG ((EventBits_t)1U)

Dispatcher::~Dispatcher() {
    // Wait for Mutex usage
    mutex.lock();
    mutex.unlock();
}

bool Dispatcher::dispatch(Function function, TickType_t timeout) {
    // Mutate
    if (mutex.lock(timeout)) {
        queue.push(std::move(function));
        if (queue.size() == BACKPRESSURE_WARNING_COUNT) {
            TT_LOG_W(TAG, "Backpressure: You're not consuming fast enough (100 queued)");
        }
        tt_check(mutex.unlock());
        // Signal
        eventFlag.set(WAIT_FLAG);
        return true;
    } else {
        TT_LOG_E(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return false;
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
                auto function = queue.front();
                queue.pop();
                consumed++;
                processing = !queue.empty();
                // Don't keep lock as callback might be slow
                mutex.unlock();
                function();
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
