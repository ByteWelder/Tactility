#pragma once

#include "I2cScanner.h"
#include <memory>

namespace tt::app::i2cscanner {

bool hasScanThread(std::shared_ptr<Data> data);
void startScanning(std::shared_ptr<Data> data);
void stopScanning(std::shared_ptr<Data> data);

}
