#pragma once

#include "service_manifest.h"

typedef void* Service;

const ServiceManifest* tt_service_get_manifest(Service service);

void tt_service_set_data(Service service, void* value);
void* _Nullable tt_service_get_data(Service service);