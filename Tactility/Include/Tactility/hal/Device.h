#pragma once

#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <vector>
#include <cassert>

namespace tt::hal {

/** Base class for HAL-related devices. */
class Device {

public:

    enum class Type {
        I2c,
        Display,
        Touch,
        SdCard,
        Keyboard,
        Power,
        Gps
    };

    typedef uint32_t Id;

private:

    Id id;

public:

    Device();
    virtual ~Device() = default;

    /** Unique identifier */
    inline Id getId() const { return id; }

    /** The type of device */
    virtual Type getType() const = 0;

    /** The part number or hardware name e.g. TdeckTouch, TdeckDisplay, BQ24295, etc. */
    virtual std::string getName() const = 0;

    /** A short description of what this device does.
     * e.g. "USB charging controller with I2C interface."
     */
    virtual std::string getDescription() const = 0;
};

/**
 * Adds a device to the registry.
 * @warning This will leak memory if you want to destroy a device and don't call deregisterDevice()!
 */
void registerDevice(const std::shared_ptr<Device>& device);

/** Remove a device from the registry. */
void deregisterDevice(const std::shared_ptr<Device>& device);

/** Find a single device with a custom filter */
std::shared_ptr<Device> _Nullable findDevice(const std::function<bool(const std::shared_ptr<Device>&)>& filterFunction);

/** Find devices with a custom filter */
std::vector<std::shared_ptr<Device>> findDevices(const std::function<bool(const std::shared_ptr<Device>&)>& filterFunction);

/** Find a device in the registry by its name. */
std::shared_ptr<Device> _Nullable findDevice(std::string name);

/** Find a device in the registry by its identifier. */
std::shared_ptr<Device> _Nullable findDevice(Device::Id id);

/** Find 0, 1 or more devices in the registry by type. */
std::vector<std::shared_ptr<Device>> findDevices(Device::Type type);

/** Get a copy of the entire device registry in its current state. */
std::vector<std::shared_ptr<Device>> getDevices();

/** Find devices of a certain type and cast them to the specified class */
template<class DeviceType>
std::vector<std::shared_ptr<DeviceType>> findDevices(Device::Type type) {
    auto devices = findDevices(type);
    if (devices.empty()) {
        return {};
    } else {
        std::vector<std::shared_ptr<DeviceType>> result;
        result.reserve(devices.size());
        for (auto& device : devices) {
            auto target_device = std::static_pointer_cast<DeviceType>(device);
            assert(target_device != nullptr);
            result.push_back(target_device);
        }
        return std::move(result);
    }
}

/** Find the first device of the specified type and cast it to the specified class */
template<class DeviceType>
std::shared_ptr<DeviceType> findFirstDevice(Device::Type type) {
    auto devices = findDevices(type);
    if (devices.empty()) {
        return {};
    } else {
        auto& first = devices[0];
        return std::static_pointer_cast<DeviceType>(first);
    }
}

}
