#pragma once

#include <TactilityCore.h>
#include <Mutex.h>
#include <Thread.h>
#include "lvgl.h"
#include "hal/i2c/I2c.h"
#include <memory>

namespace tt::app::i2cscanner {

#define TAG "i2cscanner"

enum ScanState {
    ScanStateInitial,
    ScanStateScanning,
    ScanStateStopped
};

struct Data {
    // Core
    Mutex mutex = Mutex(Mutex::TypeRecursive);
    Thread* _Nullable scanThread = nullptr;
    // State
    ScanState scanState;
    i2c_port_t port = I2C_NUM_0;
    std::vector<uint8_t> scannedAddresses;
    // Widgets
    lv_obj_t* scanButtonLabelWidget = nullptr;
    lv_obj_t* portDropdownWidget = nullptr;
    lv_obj_t* scanListWidget = nullptr;
};

void onThreadFinished(std::shared_ptr<Data> data);

}
