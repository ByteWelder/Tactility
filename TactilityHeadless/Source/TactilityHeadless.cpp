#include "Tactility/TactilityHeadless.h"
#include "Tactility/hal/Configuration.h"
#include "Tactility/hal/Hal_i.h"
#include "Tactility/network/NtpPrivate.h"
#include "Tactility/service/ServiceManifest.h"
#include "Tactility/service/ServiceRegistry.h"

#include <Tactility/Dispatcher.h>
#include <Tactility/time/TimePrivate.h>

#ifdef ESP_PLATFORM
#include "Tactility/InitEsp.h"
#endif

namespace tt {

#define TAG "tactility"

namespace service::gps { extern const ServiceManifest manifest; }
namespace service::wifi { extern const ServiceManifest manifest; }
namespace service::sdcard { extern const ServiceManifest manifest; }
#ifdef ESP_PLATFORM
namespace service::espnow { extern const ServiceManifest manifest; }
#endif

static Dispatcher mainDispatcher;

static const hal::Configuration* hardwareConfig = nullptr;

static void registerAndStartSystemServices() {
    TT_LOG_I(TAG, "Registering and starting system services");
    addService(service::gps::manifest);
    addService(service::sdcard::manifest);
    addService(service::wifi::manifest);
#ifdef ESP_PLATFORM
    addService(service::espnow::manifest);
#endif
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
    registerAndStartSystemServices();
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
