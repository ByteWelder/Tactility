#pragma once

#include "./View.h"
#include "Tactility/Timer.h"
#include <cstring>
#include <sstream>

#define TAG "SerialConsole"

namespace tt::app::serialconsole {

constexpr size_t receiveBufferSize = 512;
constexpr size_t renderBufferSize = receiveBufferSize + 2; // Leave space for newline at split and null terminator at the end

class ConsoleView final : public View {

private:

    lv_obj_t* _Nullable parent = nullptr;
    lv_obj_t* _Nullable logTextarea = nullptr;
    lv_obj_t* _Nullable inputTextarea = nullptr;
    std::unique_ptr<hal::uart::Uart> _Nullable uart = nullptr;
    std::shared_ptr<Thread> uartThread _Nullable = nullptr;
    bool uartThreadInterrupted = false;
    std::shared_ptr<Thread> viewThread _Nullable = nullptr;
    bool viewThreadInterrupted = false;
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    uint8_t receiveBuffer[receiveBufferSize];
    uint8_t renderBuffer[renderBufferSize];
    size_t receiveBufferPosition = 0;

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

        // Updating the view is expensive, so we only want to set the text once:
        // Gather all the lines in a single buffer
        if (mutex.lock()) {
            size_t first_part_size = receiveBufferSize - receiveBufferPosition;
            memcpy(renderBuffer, receiveBuffer + receiveBufferPosition, first_part_size);
            renderBuffer[receiveBufferPosition] = '\n';
            if (receiveBufferPosition > 0) {
                memcpy(renderBuffer + first_part_size + 1, receiveBuffer, (receiveBufferSize - first_part_size));
                renderBuffer[receiveBufferSize - 1] = 0x00;
            }
            mutex.unlock();
        }

        if (lvgl::lock(lvgl::defaultLockTime)) {
            lv_textarea_set_text(logTextarea, (const char*)renderBuffer);
            lvgl::unlock();
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
                mutex.lock();
                receiveBuffer[receiveBufferPosition++] = byte;
                if (receiveBufferPosition == receiveBufferSize) {
                    receiveBufferPosition = 0;
                }
                mutex.unlock();
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

    static void onSendClickedCallback(lv_event_t* event) {
        auto* view = (ConsoleView*)lv_event_get_user_data(event);
        view->onSendClicked();
    }

    void onSendClicked() {
        auto* to_send = lv_textarea_get_text(inputTextarea);

        mutex.lock();
        auto* safe_uart = uart.get();
        if (safe_uart != nullptr) {
            if (!safe_uart->writeString(to_send, 100 / portTICK_PERIOD_MS)) {
                TT_LOG_E(TAG, "Failed to send \"%s\"", to_send);
            }
        }
        mutex.unlock();

        lv_textarea_set_text(inputTextarea, "");
    }

public:

    void initLogic(std::unique_ptr<hal::uart::Uart> newUart) {
        memset(receiveBuffer, 0, receiveBufferSize);

        assert(uartThread == nullptr);
        assert(uart == nullptr);

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
    }

    void initViews(lv_obj_t* parent) {
        this->parent = parent;

        lv_obj_set_style_pad_gap(parent, 2, 0);

        logTextarea = lv_textarea_create(parent);
        lv_textarea_set_placeholder_text(logTextarea, "Waiting for data...");
        lv_obj_set_flex_grow(logTextarea, 1);
        lv_obj_set_width(logTextarea, LV_PCT(100));
        lv_obj_add_state(logTextarea, LV_STATE_DISABLED);
        lv_obj_set_style_margin_ver(logTextarea, 0, 0);
        service::gui::keyboardAddTextArea(logTextarea);

        auto* input_wrapper = lv_obj_create(parent);
        lv_obj_set_size(input_wrapper, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_pad_all(input_wrapper, 0, 0);
        lv_obj_set_style_border_width(input_wrapper, 0, 0);
        lv_obj_set_width(input_wrapper, LV_PCT(100));
        lv_obj_set_flex_flow(input_wrapper, LV_FLEX_FLOW_ROW);

        inputTextarea = lv_textarea_create(input_wrapper);
        lv_textarea_set_one_line(inputTextarea, true);
        lv_textarea_set_placeholder_text(inputTextarea, "Text to send");
        lv_obj_set_width(inputTextarea, LV_PCT(100));
        lv_obj_set_flex_grow(inputTextarea, 1);
        service::gui::keyboardAddTextArea(inputTextarea);

        auto* button = lv_button_create(input_wrapper);
        auto* button_label = lv_label_create(button);
        lv_label_set_text(button_label, "Send");
        lv_obj_add_event_cb(button, onSendClickedCallback, LV_EVENT_SHORT_CLICKED, this);

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

    void onStart(lv_obj_t* parent, std::unique_ptr<hal::uart::Uart> newUart) {
        auto lock = mutex.asScopedLock();
        lock.lock();

        initLogic(std::move(newUart));
        initViews(parent);
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
