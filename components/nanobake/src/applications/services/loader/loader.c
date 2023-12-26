#include "loader.h"
#include "core_defines.h"

static int32_t prv_loader_main(void* param) {
    UNUSED(param);
    printf("loader app init\n");
    return 0;
}

const nb_app_t loader_app = {
    .id = "loader",
    .name = "Loader",
    .type = SERVICE,
    .entry_point = &prv_loader_main,
    .stack_size = 2048,
    .priority = 10
};
