#pragma once

#include "I2cScanner.h"

namespace tt::app::i2cscanner {

bool hasScanThread(Data* data);
void startScanning(Data* data);
void stopScanning(Data* data);

}
