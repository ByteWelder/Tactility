#pragma once

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
