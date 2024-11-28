#pragma once

#include <TactilityCore.h>
#include <Mutex.h>
#include <Thread.h>
#include "lvgl.h"
#include "hal/i2c/I2c.h"

namespace tt::app::i2cscanner {

#define TAG "i2cscanner"

enum ScanState {
    ScanStateInitial,
    ScanStateScanning,
    ScanStateStopped
};

struct Data {
    // Core
    Mutex mutex = Mutex(MutexTypeRecursive);
    Thread* _Nullable scanThread = nullptr;
    // State
    ScanState scanState;
    i2c_port_t port = I2C_NUM_0;
    std::vector<uint8_t> scannedAddresses;
    // Widgets
    lv_obj_t* scanButtonLabel = nullptr;
    lv_obj_t* portDropdown = nullptr;
    lv_obj_t* scanList = nullptr;
};

void onThreadFinished(Data* data);

}
