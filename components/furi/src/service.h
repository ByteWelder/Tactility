#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "service_manifest.h"

typedef void* Service;

const ServiceManifest* service_get_manifest(Service service);

void service_set_data(Service service, void* value);
void* _Nullable service_get_data(Service service);

#ifdef __cplusplus
}
#endif