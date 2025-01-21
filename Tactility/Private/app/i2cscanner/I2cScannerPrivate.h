#pragma once

#include <TactilityCore.h>
#include <Mutex.h>
#include <Thread.h>
#include "lvgl.h"
#include "hal/i2c/I2c.h"
#include "Timer.h"
#include <memory>

namespace tt::app::i2cscanner {

#define TAG "i2cscanner"

enum ScanState {
    ScanStateInitial,
    ScanStateScanning,
    ScanStateStopped
};

struct Data {

};

void onScanTimerFinished(std::shared_ptr<Data> data);

}
