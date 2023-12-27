#include "loader.h"
#include "core_defines.h"

static int32_t prv_loader_main(void* param) {
    UNUSED(param);
    printf("loader app init\n");
    return 0;
}

const NbApp loader_app = {
    .id = "loader",
    .name = "Loader",
    .type = SERVICE,
    .entry_point = &prv_loader_main,
    .stack_size = NB_TASK_STACK_SIZE_DEFAULT,
    .priority = NB_TASK_PRIORITY_DEFAULT
};
