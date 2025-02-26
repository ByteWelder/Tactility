#pragma once

#include "../Device.h"
#include "Satellites.h"

#include <Tactility/Mutex.h>
#include <Tactility/Thread.h>

#include <minmea.h>
#include <utility>

namespace tt::hal::gps {

enum class GpsResponse {
    None,
    NotAck,
    FrameErrors,
    Ok,
};

enum class GpsModel {
    Unknown = 0,
    AG3335,
    AG3352,
    ATGM336H, // Casic (might work with AT6558, Neoway N58 LTE Cat.1, Neoway G2, Neoway G7A)
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

const char* toString(GpsModel model);

class GpsDevice : public Device {

public:

    typedef int GgaSubscriptionId;
    typedef int RmcSubscriptionId;

    struct Configuration {
        std::string name;
        std::string uartName; // e.g. "Internal" or "/dev/ttyUSB0"
        uint32_t baudRate;
        GpsModel model;
    };

    enum class State {
        PendingOn,
        On,
        Error,
        PendingOff,
        Off
    };

private:

    struct GgaSubscription {
        GgaSubscriptionId id;
        std::shared_ptr<std::function<void(Device::Id id, const minmea_sentence_gga&)>> onData;
    };

    struct RmcSubscription {
        RmcSubscriptionId id;
        std::shared_ptr<std::function<void(Device::Id id, const minmea_sentence_rmc&)>> onData;
    };

    const Configuration configuration;
    Mutex mutex = Mutex(Mutex::Type::Recursive);
    std::unique_ptr<Thread> _Nullable thread;
    bool threadInterrupted = false;
    std::vector<GgaSubscription> ggaSubscriptions;
    std::vector<RmcSubscription> rmcSubscriptions;
    GgaSubscriptionId lastSatelliteSubscriptionId = 0;
    RmcSubscriptionId lastRmcSubscriptionId = 0;
    GpsModel model = GpsModel::Unknown;
    State state = State::Off;

    static int32_t threadMainStatic(void* parameter);
    int32_t threadMain();

    bool isThreadInterrupted() const;

    void setState(State newState);

public:

    explicit GpsDevice(Configuration configuration) : configuration(std::move(configuration)) {}

    ~GpsDevice() override = default;

    Type getType() const override { return Type::Gps; }

    std::string getName() const override { return configuration.name; }
    std::string getDescription() const override { return ""; }

    bool start();
    bool stop();

    GgaSubscriptionId subscribeGga(const std::function<void(Device::Id deviceId, const minmea_sentence_gga&)>& onData) {
        auto lock = mutex.asScopedLock();
        lock.lock();
        ggaSubscriptions.push_back({
            .id = ++lastSatelliteSubscriptionId,
            .onData = std::make_shared<std::function<void(Device::Id, const minmea_sentence_gga&)>>(onData)
        });
        return lastSatelliteSubscriptionId;
    }

    void unsubscribeGga(GgaSubscriptionId subscriptionId) {
        auto lock = mutex.asScopedLock();
        lock.lock();
        std::erase_if(ggaSubscriptions, [subscriptionId](auto& subscription) { return subscription.id == subscriptionId; });
    }

    RmcSubscriptionId subscribeRmc(const std::function<void(Device::Id deviceId, const minmea_sentence_rmc&)>& onData) {
        auto lock = mutex.asScopedLock();
        lock.lock();
        rmcSubscriptions.push_back({
            .id = ++lastRmcSubscriptionId,
            .onData = std::make_shared<std::function<void(Device::Id, const minmea_sentence_rmc&)>>(onData)
        });
        return lastRmcSubscriptionId;
    }

    void unsubscribeRmc(GgaSubscriptionId subscriptionId) {
        auto lock = mutex.asScopedLock();
        lock.lock();
        std::erase_if(rmcSubscriptions, [subscriptionId](auto& subscription) { return subscription.id == subscriptionId; });
    }

    GpsModel getModel() const;

    State getState() const;
};

}
