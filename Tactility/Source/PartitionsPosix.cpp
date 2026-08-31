#ifndef ESP_PLATFORM

#include <Tactility/PartitionsPosix.h>
#include <Tactility/MountPoints.h>

#include <tactility/error.h>
#include <tactility/filesystem/file_system.h>
#include <tactility/log.h>

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

namespace tt {

constexpr auto* TAG = "Partitions";

// region file_system stub

// A plain host directory has no real mount/unmount step to perform - "mounted" here just tracks
// whether file_system_remove()'s precondition (must be unmounted first) has been satisfied.
struct DirectoryFsData {
    char path[PATH_MAX];
    bool mounted;
};

static DirectoryFsData system_fs_data;
static DirectoryFsData data_fs_data;
static FileSystem* system_fs = nullptr;
static FileSystem* data_fs = nullptr;

static error_t mount(void* data) {
    static_cast<DirectoryFsData*>(data)->mounted = true;
    return ERROR_NONE;
}

static error_t unmount(void* data) {
    static_cast<DirectoryFsData*>(data)->mounted = false;
    return ERROR_NONE;
}

static bool is_mounted(void* data) {
    return static_cast<DirectoryFsData*>(data)->mounted;
}

static error_t get_path(void* data, char* out_path, size_t out_path_size) {
    auto* fs_data = static_cast<DirectoryFsData*>(data);
    if (strlen(fs_data->path) >= out_path_size) {
        return ERROR_BUFFER_OVERFLOW;
    }
    strcpy(out_path, fs_data->path);
    return ERROR_NONE;
}

static const FileSystemApi directory_fs_api = {
    .mount = mount,
    .unmount = unmount,
    .is_mounted = is_mounted,
    .get_path = get_path,
};

// endregion file_system stub

// relative_path is resolved against the process' current working directory (the simulator is
// expected to run with Data/ as its working directory, so file::SYSTEM_PARTITION_NAME/
// DATA_PARTITION_NAME here match file::MOUNT_POINT_SYSTEM/MOUNT_POINT_DATA).
static FileSystem* registerDirectoryFs(const char* relativePath, DirectoryFsData* outData) {
    if (realpath(relativePath, outData->path) == nullptr) {
        LOG_E(TAG, "Failed to resolve '%s' to an absolute path: %s", relativePath, strerror(errno));
        return nullptr;
    }
    outData->mounted = true;
    return file_system_add(&directory_fs_api, outData);
}

static void unregisterDirectoryFs(FileSystem* fs) {
    if (fs == nullptr) {
        return;
    }
    file_system_unmount(fs);
    file_system_remove(fs);
}

bool initPartitionsPosix() {
    system_fs = registerDirectoryFs(file::SYSTEM_PARTITION_NAME, &system_fs_data);
    if (system_fs == nullptr) {
        return false;
    }

    data_fs = registerDirectoryFs(file::DATA_PARTITION_NAME, &data_fs_data);
    if (data_fs == nullptr) {
        unregisterDirectoryFs(system_fs);
        system_fs = nullptr;
        return false;
    }

    return true;
}

} // namespace

#endif // ESP_PLATFORM
