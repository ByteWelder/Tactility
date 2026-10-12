// SPDX-License-Identifier: Apache-2.0
#include <mesh_meshcore/meshcore_backend.h>
#include <mesh_meshcore/module.h>

#include <tactility/module.h>

extern "C" {

static error_t start() {
    return mesh_backend_register(&meshcore_backend_api);
}

static error_t stop() {
    return mesh_backend_unregister(&meshcore_backend_api);
}

Module mesh_meshcore_module = {
    .name = "mesh-meshcore",
    .start = start,
    .stop = stop,
    .drivers = nullptr,
    .symbols = nullptr,
    .internal = nullptr,
};

}
