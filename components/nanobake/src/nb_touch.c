#include "nb_touch.h"
#include "check.h"

nb_touch_t _Nonnull* nb_touch_alloc(nb_touch_driver_t _Nonnull* driver) {
    nb_touch_t _Nonnull* touch = malloc(sizeof(nb_touch_t));
    bool success = driver->create_touch(
        &(touch->io_handle),
        &(touch->touch_handle)
    );
    furi_check(success, "touch driver failed");
    return touch;
}
