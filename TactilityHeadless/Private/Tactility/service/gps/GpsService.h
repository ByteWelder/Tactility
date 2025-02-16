#pragma once

#include "Tactility/Mutex.h"
#include "Tactility/service/Service.h"
#include "Tactility/hal/gps/GpsDevice.h"

namespace tt::service::gps {

class GpsService final : public Service {

private:

    struct GpsDeviceRecord {
        std::shared_ptr<hal::gps::GpsDevice> device = nullptr;
        hal::gps::GpsDevice::SatelliteSubscriptionId satelliteSubscriptionId = -1;
        hal::gps::GpsDevice::LocationSubscriptionId locationSubscriptionId = -1;
    };

    minmea_sentence_rmc rmcRecord;
    TickType_t rmcTime = 0;

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::vector<GpsDeviceRecord> deviceRecords;
    bool receiving = false;

    bool startGpsDevice(GpsDeviceRecord& deviceRecord);
    static bool stopGpsDevice(GpsDeviceRecord& deviceRecord);

    GpsDeviceRecord* _Nullable findGpsRecord(const std::shared_ptr<hal::gps::GpsDevice>& record);

    void onSatelliteInfo(hal::Device::Id deviceId, const minmea_sat_info& info);
    void onRmcSentence(hal::Device::Id deviceId, const minmea_sentence_rmc& rmc);

public:

    void onStart(tt::service::ServiceContext &serviceContext) final;
    void onStop(tt::service::ServiceContext &serviceContext) final;

    void addGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);
    void removeGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);

    bool startReceiving();
    void stopReceiving();
    bool isReceiving() const;

    bool hasCoordinates() const;
    bool getCoordinates(minmea_sentence_rmc& rmc) const;
};

} // tt::hal::gps
