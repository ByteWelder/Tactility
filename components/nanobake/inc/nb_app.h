#ifndef NANOBAKE_NB_APP_H
#define NANOBAKE_NB_APP_H

#define NB_APP_ID_LENGTH 32
#define NB_APP_NAME_LENGTH 32

#include <stdio.h>
#include <esp_err.h>
#include <lvgl.h>

// region Forward declarations
struct nb_platform;
typedef struct nb_platform nb_platform_t;
//endregion

typedef enum nb_app_type nb_app_type_t;

enum nb_app_type {
    SERVICE,
    SYSTEM,
    USER
};

typedef struct nb_app nb_app_t;

typedef void (*nb_app_callback_on_create) (nb_platform_t* platform, lv_obj_t* lv_parent);
typedef void (*nb_app_callback_update) (nb_platform_t* platform, lv_obj_t* lv_parent);
typedef void (*nb_app_callback_on_destroy) (nb_platform_t* platform);

struct nb_app {
    char id[NB_APP_ID_LENGTH];
    char name[NB_APP_NAME_LENGTH];
    nb_app_type_t type;
    nb_app_callback_on_create _Nullable on_create;
    nb_app_callback_on_destroy _Nullable on_destroy;
    nb_app_callback_update _Nullable on_update;
    size_t update_task_stack_size;
    uint32_t update_task_priority;
};

typedef struct nb_app_instance nb_app_instance_t;

struct nb_app_instance {
    nb_app_t config;
};

esp_err_t nb_app_validate(nb_app_t* _Nonnull app);

#endif //NANOBAKE_NB_APP_H
