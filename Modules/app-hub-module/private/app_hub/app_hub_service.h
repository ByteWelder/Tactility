// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/manifest.h>
#include <service/manifest.h>

/** The service id, which also determines where the catalog is stored */
#define APP_HUB_SERVICE_ID "tactility.apphub"

extern const ServiceManifest app_hub_service_manifest;

/** The "apphub" command-line app */
extern "C" const AppManifest app_hub_command_manifest;

/** @return true while the background task that runs the operations exists */
bool app_hub_is_worker_running();
