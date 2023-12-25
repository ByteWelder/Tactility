#include "nb_app.h"
#include <esp_check.h>
#include <string.h>

static const char* TAG = "nb_app";

esp_err_t nb_app_validate(nb_app_t* _Nonnull app) {
    ESP_RETURN_ON_FALSE(
        strlen(app->id) < NB_APP_ID_LENGTH,
        ESP_FAIL,
        TAG,
        "app id cannot be larger than %d characters",
        NB_APP_ID_LENGTH - 1
    );

    ESP_RETURN_ON_FALSE(
        strlen(app->name) < NB_APP_NAME_LENGTH,
        ESP_FAIL,
        TAG,
        "app name cannot be larger than %d characters",
        NB_APP_NAME_LENGTH - 1
    );

    ESP_RETURN_ON_FALSE(
        (app->on_update == NULL) == (app->update_task_priority == 0 && app->update_task_priority == 0),
        ESP_FAIL,
        TAG,
        "app update is inconsistently configured"
    );

    return ESP_OK;
}
