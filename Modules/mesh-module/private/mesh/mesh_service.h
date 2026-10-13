// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <app/manifest.h>
#include <service/manifest.h>

extern const ServiceManifest mesh_service_manifest;

/** The "mesh" command-line app */
extern "C" const AppManifest mesh_command_manifest;

/** Whether the service was enabled the last time it was configured */
bool mesh_service_should_auto_start();
