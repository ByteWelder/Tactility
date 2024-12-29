#include "DispatcherThread.h"

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
    if (thread->getState() != Thread::StateStopped) {
        stop();
    }
}

void DispatcherThread::_threadMain() {
    do {
        dispatcher.consume(1000 / portTICK_PERIOD_MS);
    } while (!interruptThread);
}

void DispatcherThread::dispatch(Callback callback, std::shared_ptr<void> context) {
    dispatcher.dispatch(callback, std::move(context));
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