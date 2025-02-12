#include "Tactility/service/gps/GpsService.h"
#include "Tactility/service/ServiceManifest.h"

#define TAG "gps_service"

using tt::hal::gps::GpsDevice;

namespace tt::service::gps {

GpsService::GpsDeviceRecord* _Nullable GpsService::findGpsRecord(const std::shared_ptr<GpsDevice>& device) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    auto result = std::views::filter(deviceRecords, [&device](auto& record){
        return record.device.get() == device.get();
    });
    if (!result.empty()) {
        return &result.front();
    } else {
        return nullptr;
    }
}

void GpsService::addGpsDevice(const std::shared_ptr<GpsDevice>& device) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    GpsDeviceRecord record = { .device = device };

    if (isReceiving()) {
        startGpsDevice(record);
    }

    deviceRecords.push_back(record);
}

void GpsService::removeGpsDevice(const std::shared_ptr<GpsDevice>& device) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    GpsDeviceRecord* record = findGpsRecord(device);

    if (isReceiving()) {
        stopGpsDevice(*record);
    }

    std::erase_if(deviceRecords, [&device](auto& reference){
        return reference.device.get() == device.get();
    });
}

void GpsService::onStart(tt::service::ServiceContext &serviceContext) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    deviceRecords.clear();

    auto devices = hal::findDevices<GpsDevice>(hal::Device::Type::Gps);
    for (auto& device : devices) {
        TT_LOG_I(TAG, "[device %lu] added", device->getId());
        addGpsDevice(device);
    }
}

void GpsService::onStop(tt::service::ServiceContext &serviceContext) {
    if (isReceiving()) {
        stopReceiving();
    }
}

bool GpsService::startGpsDevice(GpsDeviceRecord& record) {
    TT_LOG_I(TAG, "[device %lu] starting", record.device->getId());

    auto lock = mutex.asScopedLock();
    lock.lock();

    auto device = record.device;

    if (!device->start()) {
        TT_LOG_E(TAG, "[device %lu] starting failed", record.device->getId());
        return false;
    }

    record.satelliteSubscriptionId = device->subscribeSatellites([this](hal::Device::Id deviceId, auto& record) {
        onSatelliteInfo(deviceId, record);
    });

    record.locationSubscriptionId = device->subscribeLocations([this](hal::Device::Id deviceId, auto& record) {
        onRmcSentence(deviceId, record);
    });

    return true;
}

bool GpsService::stopGpsDevice(GpsDeviceRecord& record) {
    TT_LOG_I(TAG, "[device %lu] stopping", record.device->getId());

    auto device = record.device;

    if (!device->stop()) {
        TT_LOG_E(TAG, "[device %lu] stopping failed", record.device->getId());
        return false;
    }

    device->unsubscribeSatellites(record.satelliteSubscriptionId);
    device->unsubscribeLocations(record.locationSubscriptionId);

    record.satelliteSubscriptionId = -1;
    record.locationSubscriptionId = -1;

    return true;
}

bool GpsService::startReceiving() {
    TT_LOG_I(TAG, "Start receiving");

    auto lock = mutex.asScopedLock();
    lock.lock();

    bool started_one_or_more = false;

    for (auto& record : deviceRecords) {
        started_one_or_more |= startGpsDevice(record);
    }

    receiving = started_one_or_more;

    return receiving;
}

void GpsService::stopReceiving() {
    TT_LOG_I(TAG, "Stop receiving");

    auto lock = mutex.asScopedLock();
    lock.lock();

    for (auto& record : deviceRecords) {
        stopGpsDevice(record);
    }

    receiving = false;
}

bool GpsService::isReceiving() {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return receiving;
}

void GpsService::onSatelliteInfo(hal::Device::Id deviceId, const minmea_sat_info& info) {
    TT_LOG_I(TAG, "[device %lu] satellite %d", deviceId, info.nr);
}

void GpsService::onRmcSentence(hal::Device::Id deviceId, const minmea_sentence_rmc& rmc) {
    TT_LOG_I(TAG, "[device %lu] coordinates %f %f", deviceId, minmea_tofloat(&rmc.longitude), minmea_tofloat(&rmc.latitude));
}

extern const ServiceManifest manifest = {
    .id = "Gps",
    .createService = create<GpsService>
};

} // tt::hal::gps
