#include <Dispatcher.h>
#include "TactilityHeadless.h"
#include "hal/Configuration.h"
#include "hal/Hal_i.h"
#include "service/ServiceManifest.h"
#include "service/ServiceRegistry.h"
#include "kernel/SystemEvents.h"
#include "network/NtpPrivate.h"
#include "time/TimePrivate.h"

#ifdef ESP_PLATFORM
#include "EspInit.h"
#endif

namespace tt {

#define TAG "tactility"

namespace service::wifi { extern const ServiceManifest manifest; }
namespace service::sdcard { extern const ServiceManifest manifest; }

static Dispatcher mainDispatcher;

static const hal::Configuration* hardwareConfig = nullptr;

static void register_and_start_system_services() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::sdcard::manifest);
    addService(service::wifi::manifest);
}

void initHeadless(const hal::Configuration& config) {
    TT_LOG_I(TAG, "Tactility v%s on %s (%s)", TT_VERSION, CONFIG_TT_BOARD_NAME, CONFIG_TT_BOARD_ID);
#ifdef ESP_PLATFORM
    initEsp();
#endif
    hardwareConfig = &config;
    time::init();
    hal::init(config);
    network::ntp::init();
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
