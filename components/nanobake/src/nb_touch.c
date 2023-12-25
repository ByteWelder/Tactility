#include "nb_touch.h"
#include "nb_assert.h"
#include <esp_check.h>

nb_touch_t _Nonnull* nb_touch_create(nb_touch_driver_t _Nonnull* driver) {
    nb_touch_t _Nonnull* touch = malloc(sizeof(nb_touch_t));
    bool success = driver->create_touch(
        &(touch->io_handle),
        &(touch->touch_handle)
    );
    NB_ASSERT(success, "touch driver failed");
    return touch;
}
