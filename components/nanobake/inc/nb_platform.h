#ifndef NANOBAKE_NB_PLATFORM_H
#define NANOBAKE_NB_PLATFORM_H

#include "nb_display.h"
#include "nb_touch.h"
#include <esp_err.h>

typedef struct nb_platform_config nb_platform_config_t;
struct nb_platform_config {
    nb_display_driver_t display_driver;
    nb_touch_driver_t touch_driver;
};

typedef struct nb_platform nb_platform_t;
struct nb_platform {
    nb_display_t display;
    nb_touch_t touch;
};

esp_err_t nb_platform_create(nb_platform_config_t config, nb_platform_t* platform);

#endif // NANOBAKE_NB_PLATFORM_H