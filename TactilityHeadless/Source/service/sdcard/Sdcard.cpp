#include "Mutex.h"
#include "Timer.h"

#include "service/ServiceContext.h"
#include "TactilityHeadless.h"
#include "service/ServiceRegistry.h"

#define TAG "sdcard_service"

namespace tt::service::sdcard {

extern const ServiceManifest manifest;

struct ServiceData {
    Mutex mutex;
    std::unique_ptr<Timer> updateTimer;
    hal::SdCard::State lastState = hal::SdCard::State::Unmounted;

    bool lock(TickType_t timeout) const {
        return mutex.acquire(timeout) == TtStatusOk;
    }

    void unlock() const {
        tt_check(mutex.release() == TtStatusOk);
    }
};

static void onUpdate(std::shared_ptr<void> context) {
    auto sdcard = tt::hal::getConfiguration()->sdcard;
    if (sdcard == nullptr) {
        return;
    }

    auto data = std::static_pointer_cast<ServiceData>(context);

    if (!data->lock(50)) {
        TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        return;
    }

    auto new_state = sdcard->getState();

    if (new_state == hal::SdCard::State::Error) {
        TT_LOG_W(TAG, "Sdcard error - unmounting. Did you eject the card in an unsafe manner?");
        sdcard->unmount();
    }

    if (new_state != data->lastState) {
        data->lastState = new_state;
    }

    data->unlock();
}

class SdCardService : public Service {

private:

    std::shared_ptr<ServiceData> data = std::make_shared<ServiceData>();

public:

    void onStart(ServiceContext& service) override {
        if (hal::getConfiguration()->sdcard != nullptr) {

            data->updateTimer = std::make_unique<Timer>(Timer::Type::Periodic, onUpdate, data);
            // We want to try and scan more often in case of startup or scan lock failure
            data->updateTimer->start(1000);
        } else {
            TT_LOG_I(TAG, "task not started due to config");
        }
    }

    void onStop(ServiceContext& service) override {
        if (data->updateTimer != nullptr) {
            // Stop thread
            data->updateTimer->stop();
            data->updateTimer = nullptr;
        }
    }
};


extern const ServiceManifest manifest = {
    .id = "sdcard",
    .createService = create<SdCardService>
};

} // namespace
