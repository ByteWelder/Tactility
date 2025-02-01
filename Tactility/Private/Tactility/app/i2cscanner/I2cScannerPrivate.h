#pragma once

#include <Tactility/hal/i2c/I2c.h>
#include <Tactility/Mutex.h>
#include <Tactility/TactilityCore.h>
#include <Tactility/Thread.h>
#include <Tactility/Timer.h>

#include <lvgl.h>
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
