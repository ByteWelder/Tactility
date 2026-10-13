// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <service/manifest.h>

/** The service id, which also determines where the catalog is stored */
#define APP_HUB_SERVICE_ID "tactility.apphub"

extern const ServiceManifest app_hub_service_manifest;

/** @return true while the background task that runs the operations exists */
bool app_hub_is_worker_running();
