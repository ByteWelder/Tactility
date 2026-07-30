#include <tactility/device.h>
#include <tactility/drivers/sdcard.h>

namespace tt::hal::sdcard {

void startAll() {
    device_for_each_of_type(&SDCARD_TYPE, nullptr, [](::Device* device, void*) -> bool {
        if (!device_is_ready(device)) {
            if (device_start(device) != ERROR_NONE) {
            }
        }
        return true;
    });
}

}
