#include <Tactility/hal/sdcard/SdCardDevice.h>
#include <Tactility/Logger.h>
#include <Tactility/LogMessages.h>
#include <Tactility/Mutex.h>
#include <Tactility/service/ServiceContext.h>
#include <Tactility/service/ServiceRegistration.h>
#include <Tactility/Tactility.h>
#include <Tactility/Timer.h>

namespace tt::service::sdcard {

static const auto LOGGER = Logger("SdcardService");

extern const ServiceManifest manifest;

class SdCardService final : public Service {

    Mutex mutex;
    std::unique_ptr<Timer> updateTimer;
    hal::sdcard::SdCardDevice::State lastState = hal::sdcard::SdCardDevice::State::Unmounted;

    bool lock(TickType_t timeout) const {
        return mutex.lock(timeout);
    }

    void unlock() const {
        mutex.unlock();
    }

    void update() {
        // TODO: Support multiple SD cards
        auto sdcard = hal::findFirstDevice<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard);
        if (sdcard == nullptr) {
            return;
        }

        if (lock(50)) {
            auto new_state = sdcard->getState();

            if (new_state == hal::sdcard::SdCardDevice::State::Error) {
                LOGGER.error("Sdcard error - unmounting. Did you eject the card in an unsafe manner?");
                sdcard->unmount();
            }

            if (new_state != lastState) {
                lastState = new_state;
            }

            unlock();
        } else {
            LOGGER.warn(LOG_MESSAGE_MUTEX_LOCK_FAILED);
        }
    }

public:

    bool onStart(ServiceContext& serviceContext) override {
        if (hal::findFirstDevice<hal::sdcard::SdCardDevice>(hal::Device::Type::SdCard) == nullptr) {
            LOGGER.warn("No SD card device found - not starting Service");
            return false;
        }

        auto service = findServiceById<SdCardService>(manifest.id);
        updateTimer = std::make_unique<Timer>(Timer::Type::Periodic, 1000, [service] {
            service->update();
        });

        // We want to try and scan more often in case of startup or scan lock failure
        updateTimer->start();

        return true;
    }

    void onStop(ServiceContext& serviceContext) override {
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
