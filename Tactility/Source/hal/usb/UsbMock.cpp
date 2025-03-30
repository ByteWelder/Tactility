#ifndef ESP_PLATFORM

#include "Tactility/hal/usb/Usb.h"

#define TAG "usb"

namespace tt::hal::usb {

bool startMassStorageWithSdmmc() { return false; }
void stop() {}
Mode getMode() { return Mode::Default; }
bool isSupported() { return false; }

bool canRebootIntoMassStorageSdmmc() { return false; }
void rebootIntoMassStorageSdmmc() {}
bool isUsbBootMode() { return false; }
void resetUsbBootMode() {}

}

#endif
