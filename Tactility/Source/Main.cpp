#include <Tactility/Tactility.h>

#include <devicetree.h>

#ifndef ESP_PLATFORM
#include <Simulator.h>
#endif

extern "C" {

void app_main() {
    tt::run(dts_modules, dts_devices);
}

} // extern