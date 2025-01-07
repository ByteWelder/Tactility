#include <Dispatcher.h>
#include "TactilityHeadless.h"
#include "hal/Configuration.h"
#include "hal/Hal_i.h"
#include "service/ServiceManifest.h"
#include "service/ServiceRegistry.h"

#ifdef ESP_PLATFORM
#include "EspInit.h"
#endif

namespace tt {

#define TAG "tactility"

namespace service::wifi { extern const ServiceManifest manifest; }
namespace service::sdcard { extern const ServiceManifest manifest; }

static Dispatcher mainDispatcher;

static const service::ServiceManifest* const system_services[] = {
    &service::sdcard::manifest,
    &service::wifi::manifest
};

static const hal::Configuration* hardwareConfig = nullptr;

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    int app_count = sizeof(system_services) / sizeof(service::ServiceManifest*);
    for (int i = 0; i < app_count; ++i) {
        addService(system_services[i]);
        tt_check(service::startService(system_services[i]->id));
    }
}

void initHeadless(const hal::Configuration& config) {
#ifdef ESP_PLATFORM
    initEsp();
#endif
    hardwareConfig = &config;
    hal::init(config);
    register_and_start_system_services();
}


Dispatcher& getMainDispatcher() {
    return mainDispatcher;
}

namespace hal {

const Configuration* getConfiguration() {
    return hardwareConfig;
}

} // namespace hal

} // namespace tt
