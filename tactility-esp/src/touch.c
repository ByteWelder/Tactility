#include "check.h"
#include "touch.h"

TouchDevice* tt_touch_alloc(TouchDriver* driver) {
    TouchDevice* touch = malloc(sizeof(TouchDevice));
    bool success = driver->create_touch_device(
        &(touch->io_handle),
        &(touch->touch_handle)
    );
    tt_check(success, "touch driver failed");
    return touch;
}
