#include "Mutex.h"
#include "Timer.h"

#include "service/ServiceContext.h"
#include "TactilityHeadless.h"
#include "service/ServiceRegistry.h"

#define TAG "sdcard_service"

namespace tt::service::sdcard {

extern const ServiceManifest manifest;

class SdCardService final : public Service {

private:

    Mutex mutex;
    std::unique_ptr<Timer> updateTimer;
    hal::SdCard::State lastState = hal::SdCard::State::Unmounted;

    bool lock(TickType_t timeout) const {
        return mutex.acquire(timeout) == TtStatusOk;
    }

    void unlock() const {
        tt_check(mutex.release() == TtStatusOk);
    }

    void update() {
        auto sdcard = tt::hal::getConfiguration()->sdcard;
        assert(sdcard);

        if (lock(50)) {
            auto new_state = sdcard->getState();

            if (new_state == hal::SdCard::State::Error) {
                TT_LOG_W(TAG, "Sdcard error - unmounting. Did you eject the card in an unsafe manner?");
                sdcard->unmount();
            }

            if (new_state != lastState) {
                lastState = new_state;
            }

            unlock();
        } else {
            TT_LOG_W(TAG, LOG_MESSAGE_MUTEX_LOCK_FAILED);
        }
    }

    static void onUpdate(std::shared_ptr<void> context) {
        auto service = std::static_pointer_cast<SdCardService>(context);
        service->update();
    }

public:

    void onStart(ServiceContext& serviceContext) final {
        if (hal::getConfiguration()->sdcard != nullptr) {
            auto service = findServiceById(manifest.id);
            updateTimer = std::make_unique<Timer>(Timer::Type::Periodic, onUpdate, service);
            // We want to try and scan more often in case of startup or scan lock failure
            updateTimer->start(1000);
        } else {
            TT_LOG_I(TAG, "Timer not started: no SD card config");
        }
    }

    void onStop(ServiceContext& serviceContext) final {
        if (updateTimer != nullptr) {
            // Stop thread
            updateTimer->stop();
            updateTimer = nullptr;
        }
    }
};

extern const ServiceManifest manifest = {
    .id = "sdcard",
    .createService = create<SdCardService>
};

} // namespace
