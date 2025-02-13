#include "Tactility/service/ServiceManifest.h"
#include "Tactility/service/ServiceRegistry.h"
#include "Tactility/service/gps/GpsService.h"

using tt::hal::gps::GpsDevice;

namespace tt::service::gps {

extern ServiceManifest manifest;

static std::shared_ptr<GpsService> findGpsService() {
    auto service = findServiceById(manifest.id);
    assert(service != nullptr);
    return std::static_pointer_cast<GpsService>(service);
}


void addGpsDevice(const std::shared_ptr<GpsDevice>& device) {
    return findGpsService()->addGpsDevice(device);
}

void removeGpsDevice(const std::shared_ptr<GpsDevice>& device) {
    return findGpsService()->removeGpsDevice(device);
}

bool startReceiving() {
    return findGpsService()->startReceiving();
}

void stopReceiving() {
    findGpsService()->stopReceiving();
}

bool isReceiving() {
    return findGpsService()->isReceiving();
}

bool hasCoordinates() {
    return findGpsService()->hasCoordinates();
}

bool getCoordinates(minmea_sentence_rmc& rmc) {
    return findGpsService()->getCoordinates(rmc);
}

} // namespace tt::service::gps
