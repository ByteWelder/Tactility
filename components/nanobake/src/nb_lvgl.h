#pragma once

#include <esp_lvgl_port.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nb_lvgl nb_lvgl_t;
struct nb_lvgl {
    lv_disp_t* _Nonnull disp;
    lv_indev_t* _Nullable touch_indev;
};

typedef struct nb_hardware nb_hardware_t;

extern nb_lvgl_t nb_lvgl_init(nb_hardware_t* platform);

#ifdef __cplusplus
}
#endif
