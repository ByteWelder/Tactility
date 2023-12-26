#pragma once

#include "nb_config.h"
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nb_hardware nb_hardware_t;
struct nb_hardware {
    nb_display_t* _Nonnull display;
    nb_touch_t* _Nullable touch;
};

/**
 * @param[in] config
 * @return a newly allocated platform instance (caller takes ownership)
 */
nb_hardware_t _Nonnull* nb_hardware_alloc(nb_config_t _Nonnull* config);

#ifdef __cplusplus
}
#endif
