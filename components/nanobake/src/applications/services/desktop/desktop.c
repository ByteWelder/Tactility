#include "desktop.h"
#include "core_defines.h"
#include "nb_hardware.h"

static int32_t prv_desktop_main(void* param) {
    UNUSED(param);
    printf("desktop app init\n");
    return 0;
}

const NbApp desktop_app = {
    .id = "desktop",
    .name = "Desktop",
    .type = SERVICE,
    .entry_point = &prv_desktop_main,
    .stack_size = NB_TASK_STACK_SIZE_DEFAULT,
    .priority = NB_TASK_PRIORITY_DEFAULT
};
