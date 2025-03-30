#pragma once

#include <cstdint>
#include <functional>

namespace tt::kernel {

enum class SystemEvent {
    BootInitHalBegin,
    BootInitHalEnd,
    BootInitI2cBegin,
    BootInitI2cEnd,
    BootInitSpiBegin,
    BootInitSpiEnd,
    BootInitUartBegin,
    BootInitUartEnd,
    BootInitLvglBegin,
    BootInitLvglEnd,
    BootSplash,
    /** Gained IP address */
    NetworkConnected,
    NetworkDisconnected,
    /** An important system time-related event, such as NTP update or time-zone change */
    Time,
};

/** Value 0 mean "no subscription" */
typedef uint32_t SystemEventSubscription;

typedef std::function<void(SystemEvent)> OnSystemEvent;

void systemEventPublish(SystemEvent event);

SystemEventSubscription systemEventAddListener(SystemEvent event, OnSystemEvent handler);

void systemEventRemoveListener(SystemEventSubscription subscription);

}