#include "Tactility/DispatcherThread.h"

namespace tt {

DispatcherThread::DispatcherThread(const std::string& threadName, size_t threadStackSize) {
    thread = std::make_unique<Thread>(
        threadName,
        threadStackSize,
        [this] {
            return threadMain();
        }
    );
}

DispatcherThread::~DispatcherThread() {
    if (thread->getState() != Thread::State::Stopped) {
        stop();
    }
}

int32_t DispatcherThread::threadMain() {
    do {
        /**
         * If this value is too high (e.g. 1 second) then the dispatcher destroys too slowly when the simulator exits.
         * This causes the problems with other services doing an update (e.g. Statusbar) and calling into destroyed mutex in the global scope.
         */
        dispatcher.consume(100 / portTICK_PERIOD_MS);
    } while (!interruptThread);

    return 0;
}

bool DispatcherThread::dispatch(Dispatcher::Function function, TickType_t timeout) {
    return dispatcher.dispatch(function, timeout);
}

void DispatcherThread::start() {
    interruptThread = false;
    thread->start();
}

void DispatcherThread::stop() {
    interruptThread = true;
    thread->join();
}

}