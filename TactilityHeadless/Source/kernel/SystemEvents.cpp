#include "Tactility/kernel/SystemEvents.h"

#include <Tactility/Mutex.h>
#include <Tactility/CoreExtraDefines.h>

#include <list>

#define TAG "system_event"

namespace tt::kernel {

struct SubscriptionData {
    SystemEventSubscription id;
    SystemEvent event;
    OnSystemEvent handler;
};

static Mutex mutex;
static SystemEventSubscription subscriptionCounter = 0;
static std::list<SubscriptionData> subscriptions;

static const char* getEventName(SystemEvent event) {
    switch (event) {
        using enum SystemEvent;
        case BootInitHalBegin:
            return TT_STRINGIFY(BootInitHalBegin);
        case BootInitHalEnd:
            return TT_STRINGIFY(BootInitHalEnd);
        case BootInitI2cBegin:
            return TT_STRINGIFY(BootInitI2cBegin);
        case BootInitI2cEnd:
            return TT_STRINGIFY(BootInitI2cEnd);
        case BootInitSpiBegin:
            return TT_STRINGIFY(BootInitSpiBegin);
        case BootInitSpiEnd:
            return TT_STRINGIFY(BootInitSpiEnd);
        case BootInitUartBegin:
            return TT_STRINGIFY(BootInitUartBegin);
        case BootInitUartEnd:
            return TT_STRINGIFY(BootInitUartEnd);
        case BootInitLvglBegin:
            return TT_STRINGIFY(BootInitLvglBegin);
        case BootInitLvglEnd:
            return TT_STRINGIFY(BootInitLvglEnd);
        case BootSplash:
            return TT_STRINGIFY(BootSplash);
        case NetworkConnected:
            return TT_STRINGIFY(NetworkConnected);
        case NetworkDisconnected:
            return TT_STRINGIFY(NetworkDisconnected);
        case Time:
            return TT_STRINGIFY(Time);
    }

    tt_crash(); // Missing case above
}

void systemEventPublish(SystemEvent event) {
    TT_LOG_I(TAG, "%s", getEventName(event));

    if (mutex.lock(portMAX_DELAY)) {
        for (auto& subscription : subscriptions) {
            if (subscription.event == event) {
                subscription.handler(event);
            }
        }

        mutex.unlock();
    }
}

SystemEventSubscription systemEventAddListener(SystemEvent event, std::function<void(SystemEvent)> handler) {
    if (mutex.lock(portMAX_DELAY)) {
        auto id = ++subscriptionCounter;

        subscriptions.push_back({
            .id = id,
            .event = event,
            .handler = handler
        });

        mutex.unlock();
        return id;
    } else {
        tt_crash();
    }
}

void systemEventRemoveListener(SystemEventSubscription subscription) {
    if (mutex.lock(portMAX_DELAY)) {
        std::erase_if(subscriptions, [subscription](auto& item) {
            return (item.id == subscription);
        });
        mutex.unlock();
    }
}

}
