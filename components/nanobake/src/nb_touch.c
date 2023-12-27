#include "nb_touch.h"
#include "check.h"

NbTouch _Nonnull* nb_touch_alloc(NbTouchDriver _Nonnull* driver) {
    NbTouch _Nonnull* touch = malloc(sizeof(NbTouch));
    bool success = driver->create_touch(
        &(touch->io_handle),
        &(touch->touch_handle)
    );
    furi_check(success, "touch driver failed");
    return touch;
}
