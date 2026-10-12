// SPDX-License-Identifier: GPL-3.0-only
#include <mesh_meshtastic/meshtastic_backend.h>
#include <mesh_meshtastic/module.h>

#include <tactility/module.h>

extern "C" {

static error_t start() {
    return mesh_backend_register(&meshtastic_backend_api);
}

static error_t stop() {
    return mesh_backend_unregister(&meshtastic_backend_api);
}

Module mesh_meshtastic_module = {
    .name = "mesh-meshtastic",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr,
};

}
