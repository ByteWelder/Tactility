#include "tt_hal_device.h"

#include "Tactility/Check.h"

#include <Tactility/hal/Device.h>

static tt::hal::Device::Type toTactilityDeviceType(DeviceType type) {
    switch (type) {
        case DEVICE_TYPE_I2C:
            return tt::hal::Device::Type::I2c;
        case DEVICE_TYPE_DISPLAY:
            return tt::hal::Device::Type::Display;
        case DEVICE_TYPE_TOUCH:
            return tt::hal::Device::Type::Touch;
        case DEVICE_TYPE_SDCARD:
            return tt::hal::Device::Type::SdCard;
        case DEVICE_TYPE_KEYBOARD:
            return tt::hal::Device::Type::Keyboard;
        case DEVICE_TYPE_POWER:
            return tt::hal::Device::Type::Power;
        case DEVICE_TYPE_GPS:
            return tt::hal::Device::Type::Gps;
        default:
            tt_crash("Device::Type not supported");
    }
}

extern "C" {

bool tt_hal_device_find(DeviceType type, DeviceId* deviceIds, uint16_t* count, uint16_t maxCount) {
    assert(maxCount > 0);

    int16_t currentIndex = -1;
    uint16_t maxIndex = maxCount - 1;

    findDevices(toTactilityDeviceType(type), [&](const std::shared_ptr<tt::hal::Device>& device) {
        currentIndex++;
        deviceIds[currentIndex] = device->getId();
        // Continue if there is storage capacity left
        return currentIndex < maxIndex;
    });

    *count = currentIndex + 1;

    return currentIndex >= 0;
}

}
