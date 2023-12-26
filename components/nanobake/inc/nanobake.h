#pragma once

#include "nb_hardware.h"
#include "nb_app.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void nanobake_start(nb_config_t _Nonnull * config);

typedef void* FuriThreadId;

extern FuriThreadId nanobake_get_app_thread_id(size_t index);
extern size_t nanobake_get_app_thread_count();

#ifdef __cplusplus
}
#endif
