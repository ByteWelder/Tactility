#pragma once

#include "../Device.h"
#include "../uart/Uart.h"
#include "GpsDeviceInitL76k.h"
#include "Satellites.h"

#include <Tactility/Mutex.h>
#include <Tactility/Thread.h>

#include <minmea.h>
#include <utility>

namespace tt::hal::gps {

class GpsDevice : public Device {

public:

    typedef int SatelliteSubscriptionId;
    typedef int LocationSubscriptionId;

    struct Configuration {
        std::string name;
        uart_port_t uartPort;
        uint32_t baudRate;
        std::function<bool(uart_port_t)> initFunction = initGpsL76k;
    };

private:

    struct SatelliteSubscription {
        SatelliteSubscriptionId id;
        std::shared_ptr<std::function<void(Device::Id id, const minmea_sat_info&)>> onData;
    };

    struct LocationSubscription {
        LocationSubscriptionId id;
        std::shared_ptr<std::function<void(Device::Id id, const minmea_sentence_rmc&)>> onData;
    };

    const Configuration configuration;
    Mutex mutex;
    std::unique_ptr<Thread> thread;
    bool threadInterrupted = false;
    std::vector<SatelliteSubscription> satelliteSubscriptions;
    std::vector<LocationSubscription> locationSubscriptions;
    SatelliteSubscriptionId lastSatelliteSubscriptionId = 0;
    LocationSubscriptionId lastLocationSubscriptionId = 0;

    static int32_t threadMainStatic(void* parameter);
    int32_t threadMain();

    bool isThreadInterrupted() const;

public:

    explicit GpsDevice(Configuration configuration) : configuration(std::move(configuration)) {
        assert(this->configuration.initFunction != nullptr);
    }

    ~GpsDevice() override = default;

    Type getType() const override { return Type::Gps; }

    std::string getName() const override { return configuration.name; }
    std::string getDescription() const override { return ""; }

    bool start();
    bool stop();

    bool isStarted() const;

    SatelliteSubscriptionId subscribeSatellites(const std::function<void(Device::Id deviceId, const minmea_sat_info&)>& onData) {
        satelliteSubscriptions.push_back({
            .id = ++lastSatelliteSubscriptionId,
            .onData = std::make_shared<std::function<void(Device::Id, const minmea_sat_info&)>>(onData)
        });
        return lastSatelliteSubscriptionId;
    }

    void unsubscribeSatellites(SatelliteSubscriptionId subscriptionId) {
        std::erase_if(satelliteSubscriptions, [subscriptionId](auto& subscription) { return subscription.id == subscriptionId; });
    }

    LocationSubscriptionId subscribeLocations(const std::function<void(Device::Id deviceId, const minmea_sentence_rmc&)>& onData) {
        locationSubscriptions.push_back({
            .id = ++lastLocationSubscriptionId,
            .onData = std::make_shared<std::function<void(Device::Id, const minmea_sentence_rmc&)>>(onData)
        });
        return lastLocationSubscriptionId;
    }

    void unsubscribeLocations(SatelliteSubscriptionId subscriptionId) {
        std::erase_if(locationSubscriptions, [subscriptionId](auto& subscription) { return subscription.id == subscriptionId; });
    }
};

}
