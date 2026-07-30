#pragma once

#include <Tactility/service/Service.h>

#include <memory>

struct Device;
struct SystemEvent;

namespace tt::service::rtctime {

class RtcTimeService final : public Service {

    bool timeEventSubscribed = false;
    Device* rtcDevice = nullptr;

    Device* findRtcDevice();
    void onTimeChanged();
    static void onTimeChangedTrampoline(struct SystemEvent* event, void* context);

public:

    bool onStart(ServiceContext& serviceContext) override;
    void onStop(ServiceContext& serviceContext) override;

    bool isAvailable() const;
};

std::shared_ptr<RtcTimeService> findRtcTimeService();

} // namespace tt::service::rtctime
