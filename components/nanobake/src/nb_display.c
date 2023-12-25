#include "nb_display.h"
#include "nb_assert.h"

nb_display_t _Nonnull* nb_display_create(nb_display_driver_t _Nonnull* driver) {
    nb_display_t _Nonnull* display = malloc(sizeof(nb_display_t));
    NB_ASSERT(driver->create_display(display) == ESP_OK, "failed to create display");
    return display;
}
