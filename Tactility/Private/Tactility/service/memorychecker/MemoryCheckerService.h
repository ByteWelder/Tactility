#pragma once

#include "Tactility/service/Service.h"

#include <Tactility/Mutex.h>
#include <Tactility/Timer.h>

namespace tt::service::memorychecker {

/**
 * Runs a background timer that validates if there's sufficient memory available.
 * It shows a statusbar icon when memory is low. It also outputs warning to the log.
 */
class MemoryCheckerService final : public Service {

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    Timer timer = Timer(Timer::Type::Periodic, [this] { onTimerUpdate(); });

    // LVGL Statusbar icon
    int8_t statusbarIconId = -1;
    // Keep track of state to minimize UI updates
    bool memoryLow = false;

    void onTimerUpdate();

public:

    bool onStart(ServiceContext& service) override;

    void onStop(ServiceContext& service) override;
};

}
