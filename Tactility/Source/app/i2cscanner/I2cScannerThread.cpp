#include "I2cScannerThread.h"
#include "lvgl.h"
#include "service/gui/Gui.h"
#include <iomanip>

namespace tt::app::i2cscanner {

static bool shouldStopScanThread(Data* data) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        bool is_scanning = data->scanState == ScanStateScanning;
        tt_check(data->mutex.release() == TtStatusOk);
        return !is_scanning;
    } else {
        return true;
    }
}

static bool getPort(Data* data, i2c_port_t* port) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        *port = data->port;
        tt_assert(data->mutex.release() == TtStatusOk);
        return true;
    } else {
        TT_LOG_W(TAG, "getPort lock");
        return false;
    }
}

static bool addAddressToList(Data* data, uint8_t address) {
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        data->scannedAddresses.push_back(address);
        tt_assert(data->mutex.release() == TtStatusOk);
        return true;
    } else {
        TT_LOG_W(TAG, "addAddressToList lock");
        return false;
    }
}

static int32_t scanThread(void* context) {
    Data* data = (Data*) context;
    TT_LOG_I(TAG, "Scan thread started");

    for (uint8_t address = 0; address < 128; ++address) {
        i2c_port_t port;
        if (getPort(data, &port)) {
            if (hal::i2c::masterCheckAddressForDevice(port, address, 10 / portTICK_PERIOD_MS)) {
                TT_LOG_I(TAG, "Found device at address %d", address);
                if (!shouldStopScanThread(data)) {
                    addAddressToList(data, address);
                }
            }
        } else {
            TT_LOG_W(TAG, "scanThread lock");
            break;
        }

        if (shouldStopScanThread(data)) {
            break;
        }
    }

    TT_LOG_I(TAG, "Scan thread finalizing");

    onThreadFinished(data);

    TT_LOG_I(TAG, "Scan thread stopped");

    return 0;
}

bool hasScanThread(Data* data) {
    bool has_thread;
    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        has_thread = data->scanThread != nullptr;
        tt_check(data->mutex.release() == TtStatusOk);
        return has_thread;
    } else {
        // Unsafe way
        TT_LOG_W(TAG, "hasScanThread lock");
        return data->scanThread != nullptr;
    }
}

void startScanning(Data* data) {
    if (hasScanThread(data)) {
        stopScanning(data);
    }

    if (data->mutex.acquire(100 / portTICK_PERIOD_MS) == TtStatusOk) {
        data->scannedAddresses.clear();

        lv_obj_add_flag(data->scanListWidget, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clean(data->scanListWidget);

        tt_assert(data->scanThread == nullptr);
        data->scanState = ScanStateScanning;
        data->scanThread = new Thread(
            "i2c scanner",
            4096,
            scanThread,
            data
        );
        data->scanThread->start();
        tt_check(data->mutex.release() == TtStatusOk);
    } else {
        TT_LOG_W(TAG, "startScanning lock");
    }
}

void stopScanning(Data* data) {
    bool sent_halt;
    if (data->mutex.acquire(250 / portTICK_PERIOD_MS) == TtStatusOk) {
        tt_assert(data->scanThread != nullptr);
        data->scanState = ScanStateStopped;
        tt_check(data->mutex.release() == TtStatusOk);
        sent_halt = true;
    } else {
        sent_halt = false;
    }

    if (sent_halt) {
        tt_assert(data->scanThread != nullptr);
        data->scanThread->join();

        if (data->mutex.acquire(250 / portTICK_PERIOD_MS) == TtStatusOk) {
            delete data->scanThread;
            data->scanThread = nullptr;
            tt_check(data->mutex.release() == TtStatusOk);
        }
    }
}

} // namespace