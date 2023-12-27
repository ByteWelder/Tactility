#include "nb_applications.h"

// System services
extern const NbApp desktop_app;
extern const NbApp gui_app;
extern const NbApp loader_app;

// System apps
extern const NbApp system_info_app;

const NbApp* const FLIPPER_SERVICES[] = {
    &desktop_app,
    &gui_app,
    &loader_app
};

const size_t FLIPPER_SERVICES_COUNT = sizeof(FLIPPER_SERVICES) / sizeof(NbApp*);

const NbApp* const FLIPPER_SYSTEM_APPS[] = {
    &system_info_app
};

const size_t FLIPPER_SYSTEM_APPS_COUNT = sizeof(FLIPPER_SYSTEM_APPS) / sizeof(NbApp*);

const NbOnSystemStart FLIPPER_ON_SYSTEM_START[] = {
};

const size_t FLIPPER_ON_SYSTEM_START_COUNT = sizeof(FLIPPER_ON_SYSTEM_START) / sizeof(NbOnSystemStart);
