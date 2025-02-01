#pragma once

namespace tt::hal::usb {

enum class Mode {
    Default, // Default state of USB stack
    None, // State after TinyUSB was used and (partially) deinitialized
    MassStorageSdmmc
};

bool startMassStorageWithSdmmc();
void stop();
Mode getMode();
bool isSupported();

bool canRebootIntoMassStorageSdmmc();
void rebootIntoMassStorageSdmmc();
bool isUsbBootMode();
void resetUsbBootMode();

}