#include "desktop.h"
#include "furi_extra_defines.h"

static int32_t prv_desktop_main(void* param) {
    UNUSED(param);
    printf("desktop app init\n");
    return 0;
}

const AppManifest desktop_app = {
    .id = "desktop",
    .name = "Desktop",
    .icon = NULL,
    .type = AppTypeService,
    .entry_point = &prv_desktop_main,
    .stack_size = AppStackSizeNormal
};
