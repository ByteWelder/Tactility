#include <cstdlib>

#include "Mutex.h"
#include "service/ServiceContext.h"
#include "TactilityCore.h"
#include "TactilityHeadless.h"

#define TAG "sdcard_service"

namespace tt::service::sdcard {

static int32_t sdcard_task(void* context);

typedef struct {
    Mutex* mutex;
    Thread* thread;
    hal::sdcard::State lastState;
    bool interrupted;
} ServiceData;

static ServiceData* service_data_alloc() {
    auto* data = static_cast<ServiceData*>(malloc(sizeof(ServiceData)));
    *data = (ServiceData) {
        .mutex = tt_mutex_alloc(MutexTypeNormal),
        .thread = new Thread(
            "sdcard",
            3000, // Minimum is ~2800 @ ESP-IDF 5.1.2 when ejecting sdcard
            &sdcard_task,
            data
        ),
        .lastState = hal::sdcard::StateUnmounted,
        .interrupted = false
    };
    data->thread->setPriority(Thread::PriorityLow);
    return data;
}

static void service_data_free(ServiceData* data) {
    tt_mutex_free(data->mutex);
    delete data->thread;
}

static void service_data_lock(ServiceData* data) {
    tt_check(tt_mutex_acquire(data->mutex, TtWaitForever) == TtStatusOk);
}

static void service_data_unlock(ServiceData* data) {
    tt_check(tt_mutex_release(data->mutex) == TtStatusOk);
}

static int32_t sdcard_task(void* context) {
    auto* data = (ServiceData*)context;
    bool interrupted = false;

    do {
        service_data_lock(data);

        interrupted = data->interrupted;

        hal::sdcard::State new_state = hal::sdcard::getState();

        if (new_state == hal::sdcard::StateError) {
            TT_LOG_W(TAG, "Sdcard error - unmounting. Did you eject the card in an unsafe manner?");
            hal::sdcard::unmount(ms_to_ticks(1000));
        }

        if (new_state != data->lastState) {
            data->lastState = new_state;
        }

        service_data_unlock(data);
        delay_ms(2000);
    } while (!interrupted);

    return 0;
}

static void on_start(ServiceContext& service) {
    if (hal::getConfiguration().sdcard != nullptr) {
        ServiceData* data = service_data_alloc();
        service.setData(data);
        data->thread->start();
    } else {
        TT_LOG_I(TAG, "task not started due to config");
    }
}

static void on_stop(ServiceContext& service) {
    auto* data = static_cast<ServiceData*>(service.getData());
    if (data != nullptr) {
        service_data_lock(data);
        data->interrupted = true;
        service_data_unlock(data);

        data->thread->join();

        service_data_free(data);
    }
}

extern const ServiceManifest manifest = {
    .id = "sdcard",
    .onStart = &on_start,
    .onStop = &on_stop
};

} // namespace
