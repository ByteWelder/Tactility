#include <Tactility/Tactility.h>

#include <devicetree.h>

#ifndef ESP_PLATFORM
#include <Simulator.h>
#endif

extern "C" {

void app_main() {
    tt::init(dts_modules, dts_devices);
    // The main task is only there to init. Delete it task afterwards to free up memory.
    vTaskDelete(nullptr);
}

} // extern
