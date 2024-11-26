#include "TactilityHeadless.h"
#include "hal/Configuration.h"
#include "hal/Hal_i.h"
#include "service/Manifest.h"
#include "service/ServiceRegistry.h"

#ifdef ESP_PLATFORM
#include "EspInit.h"
#endif

namespace tt {

#define TAG "tactility"

namespace service::wifi { extern const Manifest manifest; }
namespace service::sdcard { extern const Manifest manifest; }

static const service::Manifest* const system_services[] = {
    &service::sdcard::manifest,
    &service::wifi::manifest
};

static const hal::Configuration* hardwareConfig = nullptr;

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    int app_count = sizeof(system_services) / sizeof(service::Manifest*);
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

namespace hal {

const Configuration& getConfiguration() {
    tt_assert(hardwareConfig != nullptr);
    return *hardwareConfig;
}

} // namespace hal

} // namespace tt
