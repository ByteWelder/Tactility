#pragma once

#include "Tactility/Mutex.h"
#include "Tactility/PubSub.h"
#include "Tactility/hal/gps/GpsDevice.h"
#include "Tactility/service/Service.h"
#include "Tactility/service/ServiceContext.h"
#include "Tactility/service/gps/GpsState.h"

namespace tt::service::gps {

class GpsService final : public Service {

private:

    struct GpsDeviceRecord {
        std::shared_ptr<hal::gps::GpsDevice> device = nullptr;
        hal::gps::GpsDevice::GgaSubscriptionId satelliteSubscriptionId = -1;
        hal::gps::GpsDevice::RmcSubscriptionId rmcSubscriptionId = -1;
    };

    minmea_sentence_rmc rmcRecord;
    TickType_t rmcTime = 0;

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    Mutex stateMutex;
    std::vector<GpsDeviceRecord> deviceRecords;
    std::shared_ptr<PubSub> statePubSub = std::make_shared<PubSub>();
    std::unique_ptr<Paths> paths;
    State state = State::Off;

    bool startGpsDevice(GpsDeviceRecord& deviceRecord);
    static bool stopGpsDevice(GpsDeviceRecord& deviceRecord);

    GpsDeviceRecord* _Nullable findGpsRecord(const std::shared_ptr<hal::gps::GpsDevice>& record);

    void onGgaSentence(hal::Device::Id deviceId, const minmea_sentence_gga& gga);
    void onRmcSentence(hal::Device::Id deviceId, const minmea_sentence_rmc& rmc);

    void setState(State newState);

    void addGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);
    void removeGpsDevice(const std::shared_ptr<hal::gps::GpsDevice>& device);

    bool getConfigurationFilePath(std::string& output) const;

public:

    void onStart(tt::service::ServiceContext &serviceContext) final;
    void onStop(tt::service::ServiceContext &serviceContext) final;

    bool addGpsConfiguration(hal::gps::GpsConfiguration configuration);
    bool removeGpsConfiguration(hal::gps::GpsConfiguration configuration);
    bool getGpsConfigurations(std::vector<hal::gps::GpsConfiguration>& configurations) const;

    bool startReceiving();
    void stopReceiving();
    State getState() const;

    bool hasCoordinates() const;
    bool getCoordinates(minmea_sentence_rmc& rmc) const;

    /** @return GPS service pubsub that broadcasts State* objects */
    std::shared_ptr<PubSub> getStatePubsub() const { return statePubSub; }
};

std::shared_ptr<GpsService> findGpsService();

} // tt::service::gps
