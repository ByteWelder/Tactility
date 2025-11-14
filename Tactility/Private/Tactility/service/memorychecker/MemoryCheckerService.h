#pragma once

#include "Tactility/service/Service.h"

#include <Tactility/Mutex.h>
#include <Tactility/Timer.h>

namespace tt::service::memorychecker {

class MemoryCheckerService final : public Service {

    Mutex mutex = Mutex(Mutex::Type::Recursive);
    Timer timer = Timer(Timer::Type::Periodic, [this] {
        onTimerUpdate();
    });

    int8_t statusbarIconId = -1;
    bool memoryLow = false;

    void onTimerUpdate();
    bool isMemoryLow() const;

public:

    bool onStart(ServiceContext& service) override;

    void onStop(ServiceContext& service) override;
};

}
