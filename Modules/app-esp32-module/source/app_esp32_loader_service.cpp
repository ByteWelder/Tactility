// SPDX-License-Identifier: Apache-2.0
#include "../../../TactilityKernel/include/tactility/error.h"
#include "../../../TactilityKernel/include/tactility/filesystem/file_mutex.h"
#include "../../app-module/include/app/loader.h"
#include "../../app-module/include/app/location.h"


#include <app/loader.h>
#include <app/manifest.h>

#include <service/instance.h>
#include <service/manager.h>

#include <esp_elf.h>
#include <esp_err.h>

#include <cstdio>
#include <cstdlib>
#include <new>
#include <string>

namespace {

/** load()-allocated state, passed back through run()/unload(). */
struct Esp32AppRuntime {
    esp_elf_t elf {};
    uint8_t* file_data = nullptr;
};

error_t read_file(const char* path, uint8_t** out_data, size_t* out_size) {
    FileMutex mutex;
    file_mutex_get(&mutex, path);
    file_mutex_lock(&mutex);

    FILE* file = fopen(path, "rb");
    if (file == nullptr) {
        file_mutex_unlock(&mutex);
        return ERROR_NOT_FOUND;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    if (size <= 0) {
        fclose(file);
        file_mutex_unlock(&mutex);
        return ERROR_RESOURCE;
    }

    auto* data = static_cast<uint8_t*>(malloc(static_cast<size_t>(size)));
    if (data == nullptr) {
        fclose(file);
        file_mutex_unlock(&mutex);
        return ERROR_OUT_OF_MEMORY;
    }

    size_t read = fread(data, 1, static_cast<size_t>(size), file);
    fclose(file);
    file_mutex_unlock(&mutex);

    if (read != static_cast<size_t>(size)) {
        free(data);
        return ERROR_RESOURCE;
    }

    *out_data = data;
    *out_size = static_cast<size_t>(size);
    return ERROR_NONE;
}

error_t api_load(AppLocation location, AppRuntime* out_runtime) {
    auto* runtime = new (std::nothrow) Esp32AppRuntime();
    if (runtime == nullptr) {
        return ERROR_OUT_OF_MEMORY;
    }

    if (location.type != APP_LOCATION_PATH) {
        return ERROR_NOT_SUPPORTED;
    }

    size_t size = 0;
    error_t read_result = read_file(static_cast<const char*>(location.location), &runtime->file_data, &size);
    if (read_result != ERROR_NONE) {
        delete runtime;
        return read_result;
    }

    if (esp_elf_init(&runtime->elf) != ESP_OK) {
        free(runtime->file_data);
        delete runtime;
        return ERROR_RESOURCE;
    }

    if (esp_elf_relocate(&runtime->elf, runtime->file_data) != 0) {
        esp_elf_deinit(&runtime->elf);
        free(runtime->file_data);
        delete runtime;
        return ERROR_RESOURCE;
    }

    *out_runtime = runtime;
    return ERROR_NONE;
}

int32_t api_run(AppRuntime runtime_ptr, uint32_t /*app_instance_id*/, int argc, char* argv[]) {
    auto* runtime = static_cast<Esp32AppRuntime*>(runtime_ptr);
    // A side-loaded ELF's own main() only ever gets a real argc/argv from esp_elf_request()'s
    // fixed signature - there's no slot for app_instance_id there, and side-loaded apps don't
    // need one yet.
    return esp_elf_request(&runtime->elf, 0, argc, argv);
}

void api_unload(AppRuntime runtime_ptr) {
    auto* runtime = static_cast<Esp32AppRuntime*>(runtime_ptr);
    esp_elf_deinit(&runtime->elf);
    free(runtime->file_data);
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

extern ServiceManifest loader_service_manifest = {
    .id = APP_LOADER_PATH_SERVICE_ID,
    .create_service = create_service,
    .destroy_service = destroy_service,
    .on_start = nullptr,
    .on_stop = nullptr,
};
