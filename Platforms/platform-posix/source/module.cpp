// SPDX-License-Identifier: Apache-2.0
#include <tactility/module.h>

#include <tactility/filesystem/file_system.h>
#include <tactility/log.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

namespace {

constexpr auto* TAG = "platform-posix";

// A plain host directory has no real mount/unmount step to perform - "mounted" here just tracks
// whether file_system_remove()'s precondition (must be unmounted first) has been satisfied.
struct DirectoryFsData {
    char path[PATH_MAX];
    bool mounted;
};

DirectoryFsData system_fs_data;
DirectoryFsData data_fs_data;
FileSystem* system_fs = nullptr;
FileSystem* data_fs = nullptr;

error_t directory_fs_mount(void* data) {
    static_cast<DirectoryFsData*>(data)->mounted = true;
    return ERROR_NONE;
}

error_t directory_fs_unmount(void* data) {
    static_cast<DirectoryFsData*>(data)->mounted = false;
    return ERROR_NONE;
}

bool directory_fs_is_mounted(void* data) {
    return static_cast<DirectoryFsData*>(data)->mounted;
}

error_t directory_fs_get_path(void* data, char* out_path, size_t out_path_size) {
    auto* fs_data = static_cast<DirectoryFsData*>(data);
    if (strlen(fs_data->path) >= out_path_size) {
        return ERROR_BUFFER_OVERFLOW;
    }
    strcpy(out_path, fs_data->path);
    return ERROR_NONE;
}

const FileSystemApi directory_fs_api = {
    .mount = directory_fs_mount,
    .unmount = directory_fs_unmount,
    .is_mounted = directory_fs_is_mounted,
    .get_path = directory_fs_get_path,
};

// relative_path is resolved against the process' current working directory (the simulator is
// expected to run with Data/ as its working directory, so "system"/"data" here match
// Tactility::MountPoints.h's MOUNT_POINT_SYSTEM/MOUNT_POINT_DATA).
FileSystem* register_directory_fs(const char* relative_path, DirectoryFsData* out_data) {
    if (realpath(relative_path, out_data->path) == nullptr) {
        LOG_E(TAG, "Failed to resolve '%s' to an absolute path: %s", relative_path, strerror(errno));
        return nullptr;
    }
    out_data->mounted = true;
    return file_system_add(&directory_fs_api, out_data);
}

void unregister_directory_fs(FileSystem* fs) {
    if (fs == nullptr) {
        return;
    }
    file_system_unmount(fs);
    file_system_remove(fs);
}

} // namespace

extern "C" {

extern Driver posix_wifi_driver;

static Driver* const platform_posix_drivers[] = {
    &posix_wifi_driver,
    nullptr
};

static error_t start() {
    system_fs = register_directory_fs("system", &system_fs_data);
    if (system_fs == nullptr) {
        return ERROR_RESOURCE;
    }

    data_fs = register_directory_fs("data", &data_fs_data);
    if (data_fs == nullptr) {
        unregister_directory_fs(system_fs);
        system_fs = nullptr;
        return ERROR_RESOURCE;
    }

    return ERROR_NONE;
}

static error_t stop() {
    unregister_directory_fs(system_fs);
    unregister_directory_fs(data_fs);
    system_fs = nullptr;
    data_fs = nullptr;
    return ERROR_NONE;
}

Module platform_posix_module = {
    .name = "platform-posix",
    .start = start,
    .stop = stop,
    .drivers = platform_posix_drivers,
    .symbols = nullptr,
    .internal = nullptr,
};

}
