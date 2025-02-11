#include "Tactility/hal/gps/GpsDevice.h"
#include <minmea.h>
#include <cstring>

#define TAG "gps"
#define GPS_UART_BUFFER_SIZE 256

namespace tt::hal::gps {

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

    if (!configuration.initFunction(configuration.uartPort)) {
        TT_LOG_E(TAG, "Failed to init");
        return -1;
    }

    // Reference: https://gpsd.gitlab.io/gpsd/NMEA.html
    while (!isThreadInterrupted()) {
        if (uart::readUntil(configuration.uartPort, (uint8_t*)buffer, GPS_UART_BUFFER_SIZE, '\n', 100 / portTICK_PERIOD_MS) > 0) {
            TT_LOG_D(TAG, "RX: %s", buffer);
            switch (minmea_sentence_id((char*)buffer, false)) {
                case MINMEA_SENTENCE_RMC:
                    minmea_sentence_rmc frame;
                    if (minmea_parse_rmc(&frame, (char*)buffer)) {
                        for (auto& subscription : locationSubscriptions) {
                            (*subscription.onData)(getId(), frame);
                        }
                        TT_LOG_D(TAG, "RX RMC %f lat, %f lon, %f m/s", minmea_tocoord(&frame.latitude), minmea_tocoord(&frame.longitude), minmea_tofloat(&frame.speed));
                    } else {
                        TT_LOG_W(TAG, "RX RMC parse error: %s", buffer);
                    }
                    break;
                case MINMEA_SENTENCE_GGA:
                    minmea_sentence_gga gga_frame;
                    if (minmea_parse_gga(&gga_frame, (char*)buffer)) {
                        TT_LOG_D(TAG, "RX GGA %f lat, %f lon", minmea_tocoord(&gga_frame.latitude), minmea_tocoord(&gga_frame.longitude));
                    } else {
                        TT_LOG_W(TAG, "RX GGA parse error: %s", buffer);
                    }
                    break;
                case MINMEA_SENTENCE_GSV:
                    minmea_sentence_gsv gsv_frame;
                    if (minmea_parse_gsv(&gsv_frame, (char*)buffer)) {
                        for (auto& sat : gsv_frame.sats) {
                            if (sat.nr != 0 && sat.elevation != 0 && sat.snr != 0) {
                                for (auto& subscription : satelliteSubscriptions) {
                                    (*subscription.onData)(getId(), sat);
                                }
                            }
                            TT_LOG_D(TAG, "Satellite: id %d, elevation %d, azimuth %d, snr %d", sat.nr, sat.elevation, sat.azimuth, sat.snr);
                        }
                        TT_LOG_D(TAG, "RX GGA %f lat, %f lon", minmea_tocoord(&gga_frame.latitude), minmea_tocoord(&gga_frame.longitude));
                    } else {
                        TT_LOG_W(TAG, "RX GGA parse error: %s", buffer);
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
    auto scoped_lockable = mutex.scoped();
    scoped_lockable->lock(portMAX_DELAY);

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

    thread = std::make_unique<Thread>(
        "gps",
        4096,
        threadMainStatic,
        this
    );
    thread->setPriority(tt::Thread::Priority::High);
    thread->start();

    return true;
}

bool GpsDevice::stop() {
    auto scoped_lockable = mutex.scoped();
    scoped_lockable->lock();

    if (thread != nullptr) {
        threadInterrupted = true;

        // Detach thread, it will auto-delete when leaving the current scope
        auto old_thread = std::move(thread);

        if (old_thread->getState() != Thread::State::Stopped) {
            // Unlock so thread can lock
            scoped_lockable->unlock();
            // Wait for thread to finish
            old_thread->join();
            // Re-lock to continue logic below
            scoped_lockable->lock();
        }
    }

    if (uart::isStarted(configuration.uartPort)) {
        if (!uart::stop(configuration.uartPort)) {
            TT_LOG_E(TAG, "UART %d failed to stop", configuration.uartPort);
            return false;
        }
    }

    return true;
}

bool GpsDevice::isStarted() const {
    auto scoped_lockable = mutex.scoped();
    scoped_lockable->lock(portMAX_DELAY);
    return thread != nullptr && thread->getState() != Thread::State::Stopped;
}

bool GpsDevice::isThreadInterrupted() const {
    auto scoped_lockable = mutex.scoped();
    scoped_lockable->lock(portMAX_DELAY);
    return threadInterrupted;
}

} // namespace tt::hal::gps
