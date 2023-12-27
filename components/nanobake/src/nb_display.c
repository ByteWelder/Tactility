#include "nb_display.h"
#include "check.h"

NbDisplay _Nonnull* nb_display_alloc(NbDisplayDriver _Nonnull* driver) {
    NbDisplay _Nonnull* display = malloc(sizeof(NbDisplay));
    memset(display, 0, sizeof(NbDisplay));
    furi_check(driver->create_display(display), "failed to create display");
    furi_check(display->io_handle != NULL);
    furi_check(display->display_handle != NULL);
    furi_check(display->horizontal_resolution != 0);
    furi_check(display->vertical_resolution != 0);
    furi_check(display->draw_buffer_height > 0);
    furi_check(display->bits_per_pixel > 0);
    return display;
}
