#include "I2cScannerThread.h"
#include "lvgl.h"
#include "service/loader/Loader.h"

namespace tt::app::i2cscanner {

std::shared_ptr<Data> _Nullable optData();

static bool shouldStopScanTimer(std::shared_ptr<Data> data) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        bool is_scanning = data->scanState == ScanStateScanning;
        tt_check(data->mutex.release() == TtStatusOk);
        return !is_scanning;
    } else {
        return true;
    }
}

static bool getPort(std::shared_ptr<Data> data, i2c_port_t* port) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        *port = data->port;
        tt_assert(data->mutex.release() == TtStatusOk);
        return true;
    } else {
        TT_LOG_W(TAG, "getPort lock");
        return false;
    }
}

static bool addAddressToList(std::shared_ptr<Data> data, uint8_t address) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        data->scannedAddresses.push_back(address);
        tt_assert(data->mutex.release() == TtStatusOk);
        return true;
    } else {
        TT_LOG_W(TAG, "addAddressToList lock");
        return false;
    }
}

static void onScanTimer(TT_UNUSED std::shared_ptr<void> context) {
    auto data = optData();
    if (data == nullptr) {
        return;
    }

    TT_LOG_I(TAG, "Scan thread started");

    for (uint8_t address = 0; address < 128; ++address) {
        i2c_port_t port;
        if (getPort(data, &port)) {
            if (hal::i2c::masterCheckAddressForDevice(port, address, 10 / portTICK_PERIOD_MS)) {
                TT_LOG_I(TAG, "Found device at address %d", address);
                if (!shouldStopScanTimer(data)) {
                    addAddressToList(data, address);
                } else {
                    break;
                }
            }
        } else {
            TT_LOG_W(TAG, "onScanTimer lock");
            break;
        }

        if (shouldStopScanTimer(data)) {
            break;
        }
    }

    TT_LOG_I(TAG, "Scan thread finalizing");

    onScanTimerFinished(data);

    TT_LOG_I(TAG, "Scan timer done");
}

bool hasScanThread(std::shared_ptr<Data> data) {
    bool has_thread;
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        has_thread = data->scanTimer != nullptr;
        tt_check(data->mutex.release() == TtStatusOk);
        return has_thread;
    } else {
        // Unsafe way
        TT_LOG_W(TAG, "hasScanTimer lock");
        return data->scanTimer != nullptr;
    }
}

void startScanning(std::shared_ptr<Data> data) {
    if (hasScanThread(data)) {
        stopScanning(data);
    }

    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        data->scannedAddresses.clear();

        lv_obj_add_flag(data->scanListWidget, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clean(data->scanListWidget);

        data->scanState = ScanStateScanning;
        data->scanTimer = std::make_unique<Timer>(
            Timer::TypeOnce,
            onScanTimer,
            data
        );
        data->scanTimer->start(10);
        tt_check(data->mutex.release() == TtStatusOk);
    } else {
        TT_LOG_W(TAG, "startScanning lock");
    }
}

void stopScanning(std::shared_ptr<Data> data) {
    if (data->mutex.acquire(250 / portTICK_PERIOD_MS) == TtStatusOk) {
        tt_assert(data->scanTimer != nullptr);
        data->scanState = ScanStateStopped;
        tt_check(data->mutex.release() == TtStatusOk);
    } else {
        TT_LOG_E(TAG, "Acquire mutex failed");
    }
}

} // namespace