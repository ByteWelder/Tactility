#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/service/Service.h"

namespace tt::hal::gps {

bool init(const std::vector<GpsDevice::Configuration>& configurations) {
    for (auto& configuration : configurations) {
        auto device = std::make_shared<GpsDevice>(configuration);
        hal::registerDevice(std::move(device));
    }

    return true;
}

} // namespace tt::hal::gps
