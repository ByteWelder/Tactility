#pragma once

#include "tactility_core.h"

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((unused)) void tt_init(
    const AppManifest* const* _Nonnull apps,
    size_t apps_count,
    const ServiceManifest* const* services,
    size_t services_count
);

#ifdef __cplusplus
}
#endif
