#pragma once

namespace tt::hal::usb {

enum Mode {
    ModeDefault, // Default state of USB stack
    ModeNone, // State after TinyUSB was used and (partially) deinitialized
    ModeMassStorageSdmmc
};

bool startMassStorageWithSdmmc();
void stop();
Mode getMode();

bool canRebootIntoMassStorageSdmmc();
void rebootIntoMassStorageSdmmc();
bool isUsbBootMode();
void resetUsbBootMode();

}