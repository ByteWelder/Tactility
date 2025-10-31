#pragma once

namespace tt::hal::usb {

enum class Mode {
    Default, // Default state of USB stack
    None, // State after TinyUSB was used and (partially) deinitialized
    MassStorageSdmmc,
    MassStorageFlash  // For internal flash /data partition
};

bool startMassStorageWithSdmmc();
void stop();
Mode getMode();
bool isSupported();

bool canRebootIntoMassStorageSdmmc();
void rebootIntoMassStorageSdmmc();
bool isUsbBootMode();
void resetUsbBootMode();

// Flash-based mass storage
bool startMassStorageWithFlash();
bool canRebootIntoMassStorageFlash();
void rebootIntoMassStorageFlash();

}