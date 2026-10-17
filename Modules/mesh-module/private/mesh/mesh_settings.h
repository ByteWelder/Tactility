// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <mesh/mesh.h>

#define MESH_SERVICE_ID "tactility.mesh"

struct MeshSettings {
    bool enabled;
    char backend[MESH_BACKEND_ID_MAX_LENGTH + 1];
    /** Empty for the backend's default preset */
    char preset[MESH_NAME_MAX_LENGTH + 1];
    char node_name[MESH_NAME_MAX_LENGTH + 1];
    /** The name of the radio device, empty for the default radio */
    char radio[MESH_NAME_MAX_LENGTH + 1];
};

/** Loads the settings, or defaults when there are none */
void mesh_settings_load(struct MeshSettings* settings);

error_t mesh_settings_save(const struct MeshSettings* settings);
