#include "check.h"
#include "touch.h"

TouchDevice _Nonnull* tt_touch_alloc(TouchDriver _Nonnull* driver) {
    TouchDevice _Nonnull* touch = malloc(sizeof(TouchDevice));
    bool success = driver->create_touch_device(
        &(touch->io_handle),
        &(touch->touch_handle)
    );
    tt_check(success, "touch driver failed");
    return touch;
}
