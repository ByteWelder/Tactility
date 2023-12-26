#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nb_lvgl nb_lvgl_t;
struct nb_lvgl {
    lv_disp_t* _Nonnull disp;
    lv_indev_t* _Nullable touch_indev;
};

#ifdef __cplusplus
}
#endif
