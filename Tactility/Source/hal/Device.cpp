#include <Tactility/hal/Device.h>

#include <Tactility/Logger.h>
#include <Tactility/RecursiveMutex.h>
#include <algorithm>

namespace tt::hal {

std::vector<std::shared_ptr<Device>> devices;
RecursiveMutex mutex;
static Device::Id nextId = 0;

static const auto LOGGER = Logger("Devices");

Device::Device() : id(nextId++) {}

template <std::ranges::range RangeType>
auto toVector(RangeType&& range) {
    auto view = range | std::views::common;
    return std::vector(view.begin(), view.end());
}

void registerDevice(const std::shared_ptr<Device>& device) {
    auto scoped_mutex = mutex.asScopedLock();
    scoped_mutex.lock();

    if (findDevice(device->getId()) == nullptr) {
        devices.push_back(device);
        LOGGER.info("Registered {} with id {}", device->getName(), device->getId());
    } else {
        LOGGER.warn("Device {} with id {} was already registered", device->getName(), device->getId());
    }
}

void deregisterDevice(const std::shared_ptr<Device>& device) {
    auto scoped_mutex = mutex.asScopedLock();
    scoped_mutex.lock();

    auto id_to_remove = device->getId();
    auto remove_iterator = std::remove_if(devices.begin(), devices.end(), [id_to_remove](const auto& device) {
        return device->getId() == id_to_remove;
    });
    if (remove_iterator != devices.end()) {
        LOGGER.info("Deregistering {} with id {}", device->getName(), device->getId());
        devices.erase(remove_iterator);
    } else {
        LOGGER.warn("Deregistering {} with id {} failed: not found", device->getName(), device->getId());
    }
}

std::vector<std::shared_ptr<Device>> findDevices(const std::function<bool(const std::shared_ptr<Device>&)>& filterFunction) {
    auto scoped_mutex = mutex.asScopedLock();
    scoped_mutex.lock();

    auto devices_view = devices | std::views::filter([&filterFunction](auto& device) {
        return filterFunction(device);
    });
    return toVector(devices_view);
}

std::shared_ptr<Device> _Nullable findDevice(const std::function<bool(const std::shared_ptr<Device>&)>& filterFunction) {
    auto scoped_mutex = mutex.asScopedLock();
    scoped_mutex.lock();

    auto result_set = devices | std::views::filter([&filterFunction](auto& device) {
         return filterFunction(device);
    });
    if (!result_set.empty()) {
        return result_set.front();
    } else {
        return nullptr;
    }
}

std::shared_ptr<Device> _Nullable findDevice(std::string name) {
    return findDevice([&name](auto& device){
        return device->getName() == name;
    });
}

std::shared_ptr<Device> _Nullable findDevice(Device::Id id) {
    return findDevice([id](auto& device){
      return device->getId() == id;
    });
}

std::vector<std::shared_ptr<Device>> findDevices(Device::Type type) {
    return findDevices([type](auto& device) {
        return device->getType() == type;
    });
}

std::vector<std::shared_ptr<Device>> getDevices() {
    return devices;
}

bool hasDevice(Device::Type type) {
    auto scoped_mutex = mutex.asScopedLock();
    scoped_mutex.lock();
    auto result_set = devices | std::views::filter([&type](auto& device) {
         return device->getType() == type;
    });
    return !result_set.empty();
}

}
