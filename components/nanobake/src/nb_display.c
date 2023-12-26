#include "nb_display.h"
#include <check.h>

nb_display_t _Nonnull* nb_display_alloc(nb_display_driver_t _Nonnull* driver) {
    nb_display_t _Nonnull* display = malloc(sizeof(nb_display_t));
    furi_check(driver->create_display(display), "failed to create display");
    return display;
}
