#include "nb_touch.h"
#include "check.h"

TouchDevice _Nonnull* nb_touch_alloc(TouchDriver _Nonnull* driver) {
    TouchDevice _Nonnull* touch = malloc(sizeof(TouchDevice));
    bool success = driver->create_touch(
        &(touch->io_handle),
        &(touch->touch_handle)
    );
    furi_check(success, "touch driver failed");
    return touch;
}
