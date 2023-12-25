#include "nanobake.h"
#include "applications/main/system_info/system_info.h"

void nb_app_start(nb_platform_t _Nonnull* platform, nb_app_t _Nonnull* config) {
    lv_obj_t* scr = lv_scr_act();
    ESP_ERROR_CHECK(nb_app_validate(config));
    config->on_create(platform, scr);
}

extern void nanobake_run(nb_platform_config_t _Nonnull* config) {
    nb_platform_t _Nonnull* platform = nb_platform_create(config);

    nb_app_start(platform, config->apps[0]);
//    nb_app_start(platform, &system_info_app);
}
