#ifndef NANOBAKE_NB_PLATFORM_H
#define NANOBAKE_NB_PLATFORM_H

#include "nb_display.h"
#include "nb_touch.h"
#include "nb_app.h"
#include <esp_err.h>
#include <lvgl.h>

typedef nb_touch_driver_t (*create_touch_driver)();
typedef nb_display_driver_t (*create_display_driver)();

typedef struct nb_platform_config nb_platform_config_t;
struct nb_platform_config {
    // Required driver for display
    create_display_driver _Nonnull display_driver;
    // Optional driver for touch input
    create_touch_driver _Nullable touch_driver;
    // List of user applications
    nb_app_t* apps[];
};

typedef struct nb_lvgl nb_lvgl_t;
struct nb_lvgl {
    lv_disp_t* _Nonnull disp;
    lv_indev_t* _Nullable touch_indev;
};

typedef struct nb_platform nb_platform_t;
struct nb_platform {
    nb_display_t* _Nonnull display;
    nb_touch_t* _Nullable touch;
    nb_lvgl_t* _Nonnull lvgl;
};

/**
 * @param[in] config
 * @return a newly allocated platform instance (caller takes ownership)
 */
nb_platform_t _Nonnull* nb_platform_create(nb_platform_config_t _Nonnull* config);

#endif // NANOBAKE_NB_PLATFORM_H