#pragma once

#include "../Device.h"
#include "../uart/Uart.h"
#include "Satellites.h"

#include <Tactility/Mutex.h>
#include <Tactility/Thread.h>

#include <minmea.h>
#include <utility>

namespace tt::hal::gps {

struct GpsInfo;

typedef enum {
    GNSS_RESPONSE_NONE,
    GNSS_RESPONSE_NAK,
    GNSS_RESPONSE_FRAME_ERRORS,
    GNSS_RESPONSE_OK,
} GPS_RESPONSE;

enum class GpsModel {
    UNKNOWN = 0,
    AG3335,
    AG3352,
    ATGM336H,
    LS20031,
    MTK,
    MTK_L76B,
    MTK_PA1616S,
    UBLOX6,
    UBLOX7,
    UBLOX8,
    UBLOX9,
    UBLOX10,
    UC6580,
};

struct GpsInfo {
    GpsModel model;
    std::string software;
    std::string hardware;
    std::string firmwareVersion;
    std::string protocolVersion;
    std::string module;
    std::vector<std::string> additional;

    /** @return true if any of the std::string properties is set to a non-empty value (ignores "additional" property) */
    inline bool hasAnyData() const {
        return !software.empty() || !hardware.empty() || !firmwareVersion.empty() || !protocolVersion.empty() || !module.empty();
    }

    /** @return true if all of the std::string properties are set to a non-empty value (ignores "additional" property) */
    inline bool hasAllData() const {
        return !software.empty() && !hardware.empty() && !firmwareVersion.empty() && !protocolVersion.empty() && !module.empty();
    }

    /** Output all values to the log */
    void log() const;
};

class GpsDevice : public Device {

public:

    typedef int SatelliteSubscriptionId;
    typedef int LocationSubscriptionId;

    struct Configuration {
        std::string name;
        uart_port_t uartPort;
        uint32_t baudRate;
        GpsModel model;
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
    GpsInfo info;

    static int32_t threadMainStatic(void* parameter);
    int32_t threadMain();

    bool isThreadInterrupted() const;

public:

    explicit GpsDevice(Configuration configuration) : configuration(std::move(configuration)) {}

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

    GpsInfo getInfo() const;
};

}
