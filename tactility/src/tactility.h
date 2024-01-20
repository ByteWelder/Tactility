#pragma once

#include "app_manifest.h"
#include "service_manifest.h"

#ifdef __cplusplus
extern "C" {
#endif

TT_UNUSED void tt_init(
    const AppManifest* const* apps,
    size_t apps_count,
    const ServiceManifest* const* services,
    size_t services_count
);

#ifdef __cplusplus
}
#endif
