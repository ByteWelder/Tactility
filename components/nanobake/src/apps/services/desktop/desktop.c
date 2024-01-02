#include "desktop.h"
#include "furi_extra_defines.h"

static void desktop_start(void* param) {
    UNUSED(param);
    printf("desktop app init\n");
}

const AppManifest desktop_app = {
    .id = "desktop",
    .name = "Desktop",
    .icon = NULL,
    .type = AppTypeService,
    .on_start = &desktop_start,
    .on_stop = NULL,
    .on_show = NULL,
    .stack_size = AppStackSizeNormal
};
