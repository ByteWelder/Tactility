#pragma once

#include "nb_lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nb_hardware nb_hardware_t;

extern nb_lvgl_t nb_lvgl_init(nb_hardware_t _Nonnull* hardware);

#ifdef __cplusplus
}
#endif
