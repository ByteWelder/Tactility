#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/hal/gps/GpsInit.h"
#include "Tactility/hal/gps/Probe.h"
#include <cstring>
#include <minmea.h>

#define TAG "gps"
#define GPS_UART_BUFFER_SIZE 256

namespace tt::hal::gps {

const char* toString(GpsModel model) {
    using enum GpsModel;
    switch (model) {
        case AG3335:
            return TT_STRINGIFY(AG3335);
        case AG3352:
            return TT_STRINGIFY(AG3352);
        case ATGM336H:
            return TT_STRINGIFY(ATGM336H);
        case LS20031:
            return TT_STRINGIFY(LS20031);
        case MTK:
            return TT_STRINGIFY(MTK);
        case MTK_L76B:
            return TT_STRINGIFY(MTK_L76B);
        case MTK_PA1616S:
            return TT_STRINGIFY(MTK_PA1616S);
        case UBLOX6:
            return TT_STRINGIFY(UBLOX6);
        case UBLOX7:
            return TT_STRINGIFY(UBLOX7);
        case UBLOX8:
            return TT_STRINGIFY(UBLOX8);
        case UBLOX9:
            return TT_STRINGIFY(UBLOX9);
        case UBLOX10:
            return TT_STRINGIFY(UBLOX10);
        case UC6580:
            return TT_STRINGIFY(UC6580);
        default:
            return TT_STRINGIFY(Unknown);
    }
}

int32_t GpsDevice::threadMainStatic(void* parameter) {
    auto* gps_device = (GpsDevice*)parameter;
    return gps_device->threadMain();
}

int32_t GpsDevice::threadMain() {
    uint8_t buffer[GPS_UART_BUFFER_SIZE];

    if (!uart::setBaudRate(configuration.uartPort, (int)configuration.baudRate)) {
        TT_LOG_E(TAG, "Failed to set baud rate to %lu", configuration.baudRate);
        return -1;
    }


    GpsModel model = configuration.model;
    if (model == GpsModel::Unknown) {
        model = probe(configuration.uartPort);
        if (model == GpsModel::Unknown) {
            TT_LOG_E(TAG, "Probe failed");
            setState(State::Error);
            return -1;
        }
    }

    mutex.lock();
    this->model = model;
    mutex.unlock();

    if (!init(configuration.uartPort, model)) {
        TT_LOG_E(TAG, "Init failed");
        setState(State::Error);
        return -1;
    }

    setState(State::On);

    // Reference: https://gpsd.gitlab.io/gpsd/NMEA.html
    while (!isThreadInterrupted()) {
        size_t bytes_read = uart::readUntil(configuration.uartPort, (uint8_t*)buffer, GPS_UART_BUFFER_SIZE, '\n', 100 / portTICK_PERIOD_MS);
        if (bytes_read > 0U) {

            // Thread might've been interrupted in the meanwhile
            if (isThreadInterrupted()) {
                break;
            }

            TT_LOG_D(TAG, "%s", buffer);

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

    return 0;
}

bool GpsDevice::start() {
    auto lock = mutex.asScopedLock();
    lock.lock();

    if (thread != nullptr && thread->getState() != Thread::State::Stopped) {
        TT_LOG_W(TAG, "Already started");
        return true;
    }

    if (uart::isStarted(configuration.uartPort)) {
        TT_LOG_E(TAG, "UART %d already in use", configuration.uartPort);
        return false;
    }

    if (!uart::start(configuration.uartPort)) {
        TT_LOG_E(TAG, "UART %d failed to start", configuration.uartPort);
        return false;
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

    if (uart::isStarted(configuration.uartPort)) {
        if (!uart::stop(configuration.uartPort)) {
            TT_LOG_E(TAG, "UART %d failed to stop", configuration.uartPort);
            setState(State::Error);
            return false;
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
