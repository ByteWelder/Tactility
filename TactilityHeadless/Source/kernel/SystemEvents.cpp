#include "SystemEvents.h"
#include "Mutex.h"
#include "CoreExtraDefines.h"
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
        case SystemEvent::BootInitHalBegin:
            return TT_STRINGIFY(SystemEvent::BootInitHalBegin);
        case SystemEvent::BootInitHalEnd:
            return TT_STRINGIFY(SystemEvent::BootInitHalEnd);
        case SystemEvent::BootInitI2cBegin:
            return TT_STRINGIFY(SystemEvent::BootInitI2cBegin);
        case SystemEvent::BootInitI2cEnd:
            return TT_STRINGIFY(SystemEvent::BootInitI2cEnd);
        case SystemEvent::BootInitLvglBegin:
            return TT_STRINGIFY(SystemEvent::BootInitLvglBegin);
        case SystemEvent::BootInitLvglEnd:
            return TT_STRINGIFY(SystemEvent::BootInitLvglEnd);
        case SystemEvent::BootSplash:
            return TT_STRINGIFY(SystemEvent::BootSplash);
        case SystemEvent::NetworkConnected:
            return TT_STRINGIFY(SystemEvent::NetworkConnected);
        case SystemEvent::NetworkDisconnected:
            return TT_STRINGIFY(SystemEvent::NetworkDisconnected);
        case SystemEvent::Time:
            return TT_STRINGIFY(SystemEvent::Time);
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

SystemEventSubscription systemEventAddListener(SystemEvent event, OnSystemEvent handler) {
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
