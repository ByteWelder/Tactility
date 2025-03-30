#include "Tactility/service/gps/GpsService.h"
#include "Tactility/service/ServiceManifest.h"
#include "Tactility/service/ServiceRegistry.h"

#include <Tactility/Log.h>
#include <Tactility/file/File.h>

using tt::hal::gps::GpsDevice;

namespace tt::service::gps {

constexpr const char* TAG = "GpsService";
extern const ServiceManifest manifest;

constexpr inline bool hasTimeElapsed(TickType_t now, TickType_t timeInThePast, TickType_t expireTimeInTicks) {
    return (TickType_t)(now - timeInThePast) >= expireTimeInTicks;
}

GpsService::GpsDeviceRecord* _Nullable GpsService::findGpsRecord(const std::shared_ptr<GpsDevice>& device) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    auto result = std::views::filter(deviceRecords, [&device](auto& record) {
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

    GpsDeviceRecord record = {.device = device};

    if (getState() == State::On) { // Ignore during OnPending due to risk of data corruptiohn
        startGpsDevice(record);
    }

    deviceRecords.push_back(record);
}

void GpsService::removeGpsDevice(const std::shared_ptr<GpsDevice>& device) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    GpsDeviceRecord* record = findGpsRecord(device);

    if (getState() == State::On) { // Ignore during OnPending due to risk of data corruptiohn
        stopGpsDevice(*record);
    }

    std::erase_if(deviceRecords, [&device](auto& reference) {
        return reference.device.get() == device.get();
    });
}

void GpsService::onStart(tt::service::ServiceContext& serviceContext) {
    auto lock = mutex.asScopedLock();
    lock.lock();

    paths = serviceContext.getPaths();
}

void GpsService::onStop(tt::service::ServiceContext& serviceContext) {
    if (getState() == State::On) {
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

    record.satelliteSubscriptionId = device->subscribeGga([this](hal::Device::Id deviceId, auto& record) {
        mutex.lock();
        onGgaSentence(deviceId, record);
        mutex.unlock();
    });

    record.rmcSubscriptionId = device->subscribeRmc([this](hal::Device::Id deviceId, auto& record) {
        mutex.lock();
        if (record.longitude.value != 0 && record.longitude.scale != 0) {
            rmcRecord = record;
            rmcTime = kernel::getTicks();
        }
        onRmcSentence(deviceId, record);
        mutex.unlock();
    });

    return true;
}

bool GpsService::stopGpsDevice(GpsDeviceRecord& record) {
    TT_LOG_I(TAG, "[device %lu] stopping", record.device->getId());

    auto device = record.device;

    device->unsubscribeGga(record.satelliteSubscriptionId);
    device->unsubscribeRmc(record.rmcSubscriptionId);

    record.satelliteSubscriptionId = -1;
    record.rmcSubscriptionId = -1;

    if (!device->stop()) {
        TT_LOG_E(TAG, "[device %lu] stopping failed", record.device->getId());
        return false;
    }

    return true;
}

bool GpsService::startReceiving() {
    TT_LOG_I(TAG, "Start receiving");

    if (getState() != State::Off) {
        TT_LOG_E(TAG, "Already receiving");
        return false;
    }

    setState(State::OnPending);

    auto lock = mutex.asScopedLock();
    lock.lock();

    deviceRecords.clear();

    std::vector<hal::gps::GpsConfiguration> configurations;
    if (!getGpsConfigurations(configurations)) {
        TT_LOG_E(TAG, "Failed to get GPS configurations");
        setState(State::Off);
        return false;
    }

    if (configurations.empty()) {
        TT_LOG_E(TAG, "No GPS configurations");
        setState(State::Off);
        return false;
    }

    for (const auto& configuration: configurations) {
        auto device = std::make_shared<GpsDevice>(configuration);
        addGpsDevice(device);
    }

    bool started_one_or_more = false;

    for (auto& record: deviceRecords) {
        started_one_or_more |= startGpsDevice(record);
    }

    rmcTime = 0;

    if (started_one_or_more) {
        setState(State::On);
        return true;
    } else {
        setState(State::Off);
        return false;
    }
}

void GpsService::stopReceiving() {
    TT_LOG_I(TAG, "Stop receiving");

    setState(State::OffPending);

    auto lock = mutex.asScopedLock();
    lock.lock();

    for (auto& record: deviceRecords) {
        stopGpsDevice(record);
    }

    rmcTime = 0;

    setState(State::Off);
}

void GpsService::onGgaSentence(hal::Device::Id deviceId, const minmea_sentence_gga& gga) {
    TT_LOG_D(TAG, "[device %lu] LAT %f LON %f, satellites: %d", deviceId, minmea_tocoord(&gga.latitude), minmea_tocoord(&gga.longitude), gga.satellites_tracked);
}

void GpsService::onRmcSentence(hal::Device::Id deviceId, const minmea_sentence_rmc& rmc) {
    TT_LOG_D(TAG, "[device %lu] LAT %f LON %f, speed: %.2f", deviceId, minmea_tocoord(&rmc.latitude), minmea_tocoord(&rmc.longitude), minmea_tofloat(&rmc.speed));
}

State GpsService::getState() const {
    auto lock = stateMutex.asScopedLock();
    lock.lock();
    return state;
}

void GpsService::setState(State newState) {
    auto lock = stateMutex.asScopedLock();
    lock.lock();
    state = newState;
    lock.unlock();
    statePubSub->publish(&state);
}

bool GpsService::hasCoordinates() const {
    auto lock = mutex.asScopedLock();
    lock.lock();
    return getState() == State::On && rmcTime != 0 && !hasTimeElapsed(kernel::getTicks(), rmcTime, kernel::secondsToTicks(10));
}

bool GpsService::getCoordinates(minmea_sentence_rmc& rmc) const {
    if (hasCoordinates()) {
        rmc = rmcRecord;
        return true;
    } else {
        return false;
    }
}

std::shared_ptr<GpsService> findGpsService() {
    auto service = findServiceById(manifest.id);
    assert(service != nullptr);
    return std::static_pointer_cast<GpsService>(service);
}

extern const ServiceManifest manifest = {
    .id = "Gps",
    .createService = create<GpsService>
};

} // namespace tt::service::gps
