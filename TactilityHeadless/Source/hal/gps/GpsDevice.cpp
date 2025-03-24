#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/gps/GpsInit.h"
#include "Tactility/hal/gps/Probe.h"
#include "Tactility/hal/uart/Uart.h"
#include <cstring>
#include <minmea.h>

namespace tt::hal::gps {

constexpr uint32_t GPS_UART_BUFFER_SIZE = 256;
constexpr const char* TAG = "GpsDevice";

int32_t GpsDevice::threadMainStatic(void* parameter) {
    auto* gps_device = (GpsDevice*)parameter;
    return gps_device->threadMain();
}

int32_t GpsDevice::threadMain() {
    uint8_t buffer[GPS_UART_BUFFER_SIZE];

    auto uart = uart::open(configuration.uartName);
    if (uart == nullptr) {
        TT_LOG_E(TAG, "Failed to open UART %s", configuration.uartName);
        return -1;
    }

    if (!uart->start()) {
        TT_LOG_E(TAG, "Failed to start UART %s", configuration.uartName);
        return -1;
    }

    if (!uart->setBaudRate((int)configuration.baudRate)) {
        TT_LOG_E(TAG, "Failed to set baud rate to %lu for UART %s", configuration.baudRate, configuration.uartName);
        return -1;
    }

    GpsModel model = configuration.model;
    if (model == GpsModel::Unknown) {
        model = probe(*uart);
        if (model == GpsModel::Unknown) {
            TT_LOG_E(TAG, "Probe failed");
            setState(State::Error);
            return -1;
        }
    }
    mutex.lock();
    this->model = model;
    mutex.unlock();

    if (!init(*uart, model)) {
        TT_LOG_E(TAG, "Init failed");
        setState(State::Error);
        return -1;
    }

    setState(State::On);

    // Reference: https://gpsd.gitlab.io/gpsd/NMEA.html
    while (!isThreadInterrupted()) {
        size_t bytes_read = uart->readUntil(reinterpret_cast<std::byte*>(buffer), GPS_UART_BUFFER_SIZE, '\n', 100 / portTICK_PERIOD_MS);

        // Thread might've been interrupted in the meanwhile
        if (isThreadInterrupted()) {
            break;
        }

        if (bytes_read > 0U) {

            TT_LOG_I(TAG, "[%ul] %s", bytes_read, buffer);

            switch (minmea_sentence_id((char*)buffer, false)) {
                case MINMEA_SENTENCE_RMC:
                    minmea_sentence_rmc rmc_frame;
                    if (minmea_parse_rmc(&rmc_frame, (char*)buffer)) {
                        mutex.lock();
                        for (auto& subscription : rmcSubscriptions) {
                            (*subscription.onData)(getId(), rmc_frame);
                        }
                        mutex.unlock();
                        TT_LOG_D(TAG, "RMC %f lat, %f lon, %f m/s", minmea_tocoord(&rmc_frame.latitude), minmea_tocoord(&rmc_frame.longitude), minmea_tofloat(&rmc_frame.speed));
                    } else {
                        TT_LOG_W(TAG, "RMC parse error: %s", buffer);
                    }
                    break;
                case MINMEA_SENTENCE_GGA:
                    minmea_sentence_gga gga_frame;
                    if (minmea_parse_gga(&gga_frame, (char*)buffer)) {
                        mutex.lock();
                        for (auto& subscription : ggaSubscriptions) {
                            (*subscription.onData)(getId(), gga_frame);
                        }
                        mutex.unlock();
                        TT_LOG_D(TAG, "GGA %f lat, %f lon", minmea_tocoord(&gga_frame.latitude), minmea_tocoord(&gga_frame.longitude));
                    } else {
                        TT_LOG_W(TAG, "GGA parse error: %s", buffer);
                    }
                    break;
                default:
                    break;
            }
        }
    }

    if (uart->isStarted() && !uart->stop()) {
        TT_LOG_W(TAG, "Failed to stop UART %s", configuration.uartName);
    }

    return 0;
}

bool GpsDevice::start() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (thread != nullptr && thread->getState() != Thread::State::Stopped) {
        TT_LOG_W(TAG, "Already started");
        return true;
    }

    threadInterrupted = false;

    TT_LOG_I(TAG, "Starting thread");
    setState(State::PendingOn);

    thread = std::make_unique<Thread>(
        "gps",
        4096,
        threadMainStatic,
        this
    );
    thread->setPriority(tt::Thread::Priority::High);
    thread->start();

    TT_LOG_I(TAG, "Starting finished");
    return true;
}

bool GpsDevice::stop() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    setState(State::PendingOff);

    if (thread != nullptr) {
        threadInterrupted = true;

        // Detach thread, it will auto-delete when leaving the current scope
        auto old_thread = std::move(thread);

        if (old_thread->getState() != Thread::State::Stopped) {
            // Unlock so thread can lock
            lock.unlock();
            // Wait for thread to finish
            old_thread->join();
            // Re-lock to continue logic below
            lock.lock();
        }
    }

    setState(State::Off);

    return true;
}

bool GpsDevice::isThreadInterrupted() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return threadInterrupted;
}

GpsModel GpsDevice::getModel() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return model; // Make copy because of thread safety
}

GpsDevice::State GpsDevice::getState() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return state; // Make copy because of thread safety
}

void GpsDevice::setState(State newState) {
    auto lock = mutex.asScopedLock();
    lock.lock();
    state = newState;
}

} // namespace tt::hal::gps
