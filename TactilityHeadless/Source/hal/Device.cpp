#include "Tactility/hal/Device.h"

#include <ranges>
#include <Tactility/Mutex.h>

namespace tt::hal {

std::vector<std::shared_ptr<Device>> devices;
Mutex mutex = Mutex(Mutex::Type::Recursive);
static Device::Id nextId = 0;

#define TAG "devices"

Device::Device() : id(nextId++) {}

template <std::ranges::range RangeType>
auto toVector(RangeType&& range) {
    auto view = range | std::views::common;
    return std::vector(view.begin(), view.end());
}

void registerDevice(const std::shared_ptr<Device>& device) {
    auto scoped_mutex = mutex.scoped();
    if (scoped_mutex->lock()) {
        if (findDevice(device->getId()) == nullptr) {
            devices.push_back(device);
            TT_LOG_I(TAG, "Registered %s with id %lu", device->getName().c_str(), device->getId());
        } else {
            TT_LOG_W(TAG, "Device %s with id %lu was already registered", device->getName().c_str(), device->getId());
        }
    }
}

void unregisterDevice(const std::shared_ptr<Device>& device) {
    auto scoped_mutex = mutex.scoped();
    auto id_to_remove = device->getId();
    if (scoped_mutex->lock()) {
        auto remove_iterator = std::remove_if(devices.begin(), devices.end(), [id_to_remove](const auto& device) {
            return device->getId() == id_to_remove;
        });
        devices.erase(remove_iterator);
    }
}

std::vector<std::shared_ptr<Device>> findDevices(const std::function<bool(const std::shared_ptr<Device>&)>& filterFunction) {
    auto scoped_mutex = mutex.scoped();
    if (scoped_mutex->lock()) {
        auto devices_view = devices | std::views::filter([&filterFunction](auto& device) {
            return filterFunction(device);
        });
        return toVector(devices_view);
    }
    return {};
}

static std::shared_ptr<Device> _Nullable findDevice(const std::function<bool(const std::shared_ptr<Device>&)>& filterFunction) {
    auto scoped_mutex = mutex.scoped();
    if (scoped_mutex->lock()) {
        auto result_set = devices | std::views::filter([&filterFunction](auto& device) {
             return filterFunction(device);
        });
        if (!result_set.empty()) {
            return result_set.front();
        }
    }

    return nullptr;
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

}
