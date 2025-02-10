#include "Tactility/hal/gps/Gps.h"

namespace tt::hal::gps {

bool init(const std::vector<GpsDevice::Configuration>& configurations) {
    for (auto& configuration : configurations) {
        auto device = std::make_shared<GpsDevice>(configuration);
        device->start(); // TODO: Remove
        hal::registerDevice(std::move(device));
    }

    return true;
}

}