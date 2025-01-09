#pragma once

#include <cstdint>

namespace tt::kernel {

enum class SystemEvent {
    BootInitHalBegin,
    BootInitHalEnd,
    BootInitI2cBegin,
    BootInitI2cEnd,
    BootInitLvglBegin,
    BootInitLvglEnd,
    BootSplash,
    /** Gained IP address */
    NetworkConnected,
    NetworkDisconnected,
    NtpSynced
};

/** Value 0 mean "no subscription" */
typedef uint32_t SystemEventSubscription;

typedef void (*OnSystemEvent)(SystemEvent event);

void systemEventPublish(SystemEvent event);
SystemEventSubscription systemEventAddListener(SystemEvent event, OnSystemEvent handler);
void systemEventRemoveListener(SystemEventSubscription subscription);

}