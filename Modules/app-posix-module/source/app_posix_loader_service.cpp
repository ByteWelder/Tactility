// SPDX-License-Identifier: Apache-2.0
#include <app/loader.h>
#include <app/location.h>

#include <tactility/error.h>
#include <tactility/log.h>

#include <service/manager.h>

#include <dlfcn.h>
#include <sys/stat.h>

#include <new>
#include <string>

constexpr auto* TAG = "app_posix_loader";

namespace {

/** load()-allocated state, passed back through run()/unload(). */
struct PosixAppRuntime {
    void* handle = nullptr;
};

bool is_regular_file(const std::string& path) {
    struct stat path_stat {};
    return ::stat(path.c_str(), &path_stat) == 0 && S_ISREG(path_stat.st_mode);
}

// location.location can be either an app's install directory or the .so file directly; the
// former resolves to the per-architecture binary at {dir}/elf/posix-{TACTILITY_POSIX_ARCH}.so,
// mirroring app_esp32_loader_service.cpp's resolve_elf_path().
error_t resolve_app_path(const std::string& path, std::string& resolvedPath) {
    if (path.ends_with(".so")) {
        resolvedPath = path;
        return ERROR_NONE;
    }
    std::string shared_object_path = path + "/elf/posix-" TACTILITY_POSIX_ARCH ".so";
    if (!is_regular_file(shared_object_path)) {
        return ERROR_NOT_FOUND;
    }
    resolvedPath = shared_object_path;
    return ERROR_NONE;
}

error_t api_load(AppLocation location, AppRuntime* out_runtime) {
    if (location.type != APP_LOCATION_PATH) {
        LOG_E(TAG, "Unsupported location type");
        return ERROR_NOT_SUPPORTED;
    }

    std::string app_path;
    auto error = resolve_app_path(static_cast<const char*>(location.location), app_path);
    if (error != ERROR_NONE) {
        LOG_E(TAG, "Failed to resolve app path: %s", location.location);
        return error;
    }

    LOG_I(TAG, "Loading %s", app_path.c_str());

    // RTLD_NOW: a missing symbol fails here, not mid-run(). RTLD_LOCAL: this app's own exported
    // symbols (if any beyond its entry point) don't leak into the process's global scope and
    // clash with a different app's.
    void* handle = dlopen(app_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        LOG_E(TAG, "dlopen(%s) failed: %s", app_path.c_str(), dlerror());
        return ERROR_NOT_FOUND;
    }

    auto* runtime = new (std::nothrow) PosixAppRuntime { .handle = handle };
    if (runtime == nullptr) {
        LOG_E(TAG, "Out of memory");
        dlclose(handle);
        return ERROR_OUT_OF_MEMORY;
    }

    *out_runtime = runtime;
    return ERROR_NONE;
}

int32_t api_run(AppRuntime runtime_ptr, uint32_t /*app_instance_id*/, int argc, char* argv[]) {
    auto* runtime = static_cast<PosixAppRuntime*>(runtime_ptr);

    dlerror(); // clear any pending error, per dlsym(3)'s own recommended idiom for telling a NULL
               // symbol address apart from a real lookup failure
    void* symbol = dlsym(runtime->handle, "main");
    const char* lookup_error = dlerror();
    if (symbol == nullptr || lookup_error != nullptr) {
        LOG_E(TAG, "dlsym(\"main\") failed: %s", lookup_error != nullptr ? lookup_error : "not found");
        return -1;
    }

    auto* main_fn = reinterpret_cast<AppMainFn>(symbol);
    return main_fn(argc, argv);
}

void api_unload(AppRuntime runtime_ptr) {
    auto* runtime = static_cast<PosixAppRuntime*>(runtime_ptr);
    dlclose(runtime->handle);
    delete runtime;
}

AppLoaderApi loader_api = {
    .load = api_load,
    .run = api_run,
    .unload = api_unload,
};

void* create_service(const ServiceManifest*) {
    return &loader_api;
}

void destroy_service(const ServiceManifest*, void*) {
}

} // namespace

ServiceManifest loader_service_manifest = {
    .id = APP_LOADER_PATH_SERVICE_ID,
    .create_service = create_service,
    .destroy_service = destroy_service,
    .on_start = nullptr,
    .on_stop = nullptr,
};
