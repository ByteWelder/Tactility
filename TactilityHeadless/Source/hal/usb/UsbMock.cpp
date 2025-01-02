#ifndef ESP_PLATFORM

#include "Usb.h"

#define TAG "usb"

namespace tt::hal::usb {

bool startMassStorageWithSdmmc() { return false; }
void stop() {}
Mode getMode() { return ModeDefault; }

bool canRebootIntoMassStorageSdmmc() { return false; }
void rebootIntoMassStorageSdmmc() {}
bool isUsbBootMode() { return false; }
void resetUsbBootMode() {}

}

#endif
