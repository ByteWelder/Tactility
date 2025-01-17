#pragma once

#include "./I2cScannerPrivate.h"
#include <memory>

namespace tt::app::i2cscanner {

bool hasScanTimer(std::shared_ptr<Data> data);
void startScanning(std::shared_ptr<Data> data);
void stopScanning(std::shared_ptr<Data> data);

}
