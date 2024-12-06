#include <cstdlib>

#include "Mutex.h"
#include "service/ServiceContext.h"
#include "TactilityCore.h"
#include "TactilityHeadless.h"
#include "service/ServiceRegistry.h"

#define TAG "sdcard_service"

namespace tt::service::sdcard {

static int32_t sdcard_task(void* context);
extern const ServiceManifest manifest;

struct ServiceData {
    Mutex mutex;
    Thread thread = Thread(
        "sdcard",
        3000, // Minimum is ~2800 @ ESP-IDF 5.1.2 when ejecting sdcard
        &sdcard_task,
        nullptr
    );
    hal::sdcard::State lastState = hal::sdcard::StateUnmounted;
    bool interrupted = false;

    ServiceData() {
        thread.setPriority(Thread::PriorityLow);
    }

    void lock() const {
        tt_check(mutex.acquire(TtWaitForever) == TtStatusOk);
    }

    void unlock() const {
        tt_check(mutex.release() == TtStatusOk);
    }
};


static int32_t sdcard_task(TT_UNUSED void* context) {
    delay_ms(20); // TODO: Make service instance findable earlier on (but expose "starting" state?)
    auto service = findServiceById(manifest.id);
    if (service == nullptr) {
        TT_LOG_E(TAG, "Service not found");
        return -1;
    }

    auto data = std::static_pointer_cast<ServiceData>(service->getData());

    bool interrupted = false;

    do {
        data->lock();

        interrupted = data->interrupted;

        hal::sdcard::State new_state = hal::sdcard::getState();

        if (new_state == hal::sdcard::StateError) {
            TT_LOG_W(TAG, "Sdcard error - unmounting. Did you eject the card in an unsafe manner?");
            hal::sdcard::unmount(ms_to_ticks(1000));
        }

        if (new_state != data->lastState) {
            data->lastState = new_state;
        }

        data->lock();
        delay_ms(2000);
    } while (!interrupted);

    return 0;
}

static void onStart(ServiceContext& service) {
    if (hal::getConfiguration().sdcard != nullptr) {
        auto data = std::make_shared<ServiceData>();
        service.setData(data);
        data->thread.start();
    } else {
        TT_LOG_I(TAG, "task not started due to config");
    }
}

static void onStop(ServiceContext& service) {
    auto data = std::static_pointer_cast<ServiceData>(service.getData());
    if (data != nullptr) {
        data->lock();
        data->interrupted = true;
        data->unlock();

        data->thread.join();
    }
}

extern const ServiceManifest manifest = {
    .id = "sdcard",
    .onStart = onStart,
    .onStop = onStop
};

} // namespace
