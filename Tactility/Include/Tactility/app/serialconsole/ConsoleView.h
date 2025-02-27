#pragma once

#include "./View.h"
#include "Tactility/Timer.h"
#include <deque>

namespace tt::app::serialconsole {

class ConsoleView final : public View {

private:

    lv_obj_t* _Nullable parent = nullptr;
    lv_obj_t* _Nullable logTextarea = nullptr;
    std::unique_ptr<hal::uart::Uart> _Nullable uart = nullptr;
    std::shared_ptr<Thread> uartThread _Nullable = nullptr;
    bool uartThreadInterrupted = false;
    std::shared_ptr<Thread> viewThread _Nullable = nullptr;
    bool viewThreadInterrupted = false;
    Mutex mutex = Mutex(Mutex::Type::Recursive);

    bool isUartThreadInterrupted() const {
        auto lock = mutex.asScopedLock();
        lock.lock();
        return uartThreadInterrupted;
    }

    bool isViewThreadInterrupted() const {
        auto lock = mutex.asScopedLock();
        lock.lock();
        return viewThreadInterrupted;
    }

    void updateViews() {
        auto lvgl_lock = lvgl::getSyncLock()->asScopedLock();
        if (!lvgl_lock.lock(lvgl::defaultLockTime)) {
            return;
        }

        if (parent == nullptr) {
            return;
        }

    }

    int32_t viewThreadMain() {
        while (!isViewThreadInterrupted()) {
            auto start_time = kernel::getTicks();

            updateViews();

            auto end_time = kernel::getTicks();
            auto time_diff = end_time - start_time;
            if (time_diff < 500U) {
                kernel::delayTicks((500U - time_diff) / portTICK_PERIOD_MS);
            }
        }

        return 0;
    }

    int32_t uartThreadMain() {
        uint8_t byte;

        while (!isUartThreadInterrupted()) {
            bool success = uart->readByte(&byte, 50 / portTICK_PERIOD_MS);

            // Thread might've been interrupted in the meanwhile
            if (isUartThreadInterrupted()) {
                break;
            }

            if (success) {
                lvgl::lock(lvgl::defaultLockTime);
                if (byte >= 0x21U && byte <= 0x7EU) {
                    lv_textarea_add_char(logTextarea, byte);
                } else if (byte == '\n') {
                    lv_textarea_add_char(logTextarea, byte);
                } else {
                    lv_textarea_add_text(logTextarea, std::format("0x%02H ", byte).c_str());
                }
                lvgl::unlock();
            }

        }

        return 0;
    }

    static int32_t viewThreadMainStatic(void* parameter) {
        auto* view = (ConsoleView*)parameter;
        return view->viewThreadMain();
    }

    static int32_t uartThreadMainStatic(void* parameter) {
        auto* view = (ConsoleView*)parameter;
        return view->uartThreadMain();
    }

public:

    void onStart(lv_obj_t* parent, std::unique_ptr<hal::uart::Uart> newUart) {
        auto lock = mutex.asScopedLock();
        lock.lock();

        assert(uartThread == nullptr);
        assert(uart == nullptr);

        this->parent = parent;

        logTextarea = lv_textarea_create(parent);

        uart = std::move(newUart);

        uartThreadInterrupted = false;
        uartThread = std::make_unique<Thread>(
            "SerConsUart",
            4096,
            uartThreadMainStatic,
            this
        );
        uartThread->setPriority(tt::Thread::Priority::High);
        uartThread->start();

        viewThreadInterrupted = false;
        viewThread = std::make_unique<Thread>(
            "SerConsView",
            4096,
            viewThreadMainStatic,
            this
        );
        viewThread->setPriority(THREAD_PRIORITY_RENDER);
        viewThread->start();
    }

    void onStop() final {
        auto lock = mutex.asScopedLock();
        lock.lock();

        uartThreadInterrupted = true;
        viewThreadInterrupted = true;

        // Detach thread, it will auto-delete when leaving the current scope
        auto old_uart_thread = std::move(uartThread);

        if (old_uart_thread->getState() != Thread::State::Stopped) {
            // Unlock so thread can lock
            lock.unlock();
            // Wait for thread to finish
            old_uart_thread->join();
        }

        lock.lock();

        // Detach thread, it will auto-delete when leaving the current scope
        auto old_view_thread = std::move(viewThread);

        if (old_view_thread->getState() != Thread::State::Stopped) {
            // Unlock so thread can lock
            lock.unlock();
            // Wait for thread to finish
            old_view_thread->join();
        }
    }

    void onDisconnect() {
        auto lock = mutex.asScopedLock();
        lock.lock();

        if (uart != nullptr && uart->isStarted()) {
            uart->stop();
            uart = nullptr;
        }
    }
};

} // namespace tt::app::serialconsole
