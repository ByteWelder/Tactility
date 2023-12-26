#include "gui.h"
#include "core_defines.h"
#include "check.h"

static int32_t prv_gui_main(void* param) {
    UNUSED(param);
    printf("gui app init\n");
    return 0;
}

const nb_app_t gui_app = {
    .id = "gui",
    .name = "GUI",
    .type = SERVICE,
    .entry_point = &prv_gui_main,
    .stack_size = 2048,
    .priority = 10
};
