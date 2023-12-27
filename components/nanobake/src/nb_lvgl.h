#pragma once

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_disp_t* _Nonnull disp;
    lv_indev_t* _Nullable touch_indev;
} NbLvgl;

#ifdef __cplusplus
}
#endif
