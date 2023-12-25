#include "nb_touch.h"
#include <esp_check.h>

nb_touch_t _Nonnull* nb_touch_create(nb_touch_driver_t _Nonnull* driver) {
    nb_touch_t _Nonnull* touch = malloc(sizeof(nb_touch_t));
    assert(driver->init_io(&(touch->io_handle)) == ESP_OK);
    driver->create_touch(touch->io_handle, &(touch->touch_handle));
    return touch;
}
