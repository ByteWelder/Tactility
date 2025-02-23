#include "Tactility/DispatcherThread.h"

namespace tt {

int32_t dispatcherThreadMain(void* context) {
    auto* dispatcherThread = (DispatcherThread*)context;
    dispatcherThread->_threadMain();
    return 0;
}

DispatcherThread::DispatcherThread(const std::string& threadName, size_t threadStackSize) {
    thread = std::make_unique<Thread>(
        threadName,
        threadStackSize,
        dispatcherThreadMain,
        this
    );
}

DispatcherThread::~DispatcherThread() {
    if (thread->getState() != Thread::State::Stopped) {
        stop();
    }
}

void DispatcherThread::_threadMain() {
    do {
        /**
         * If this value is too high (e.g. 1 second) then the dispatcher destroys too slowly when the simulator exits.
         * This causes the problems with other services doing an update (e.g. Statusbar) and calling into destroyed mutex in the global scope.
         */
        dispatcher.consume(100 / portTICK_PERIOD_MS);
    } while (!interruptThread);
}

void DispatcherThread::dispatch(Dispatcher::Function function, std::shared_ptr<void> context, TickType_t timeout) {
    dispatcher.dispatch(function, std::move(context), timeout);
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