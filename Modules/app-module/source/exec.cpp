// SPDX-License-Identifier: Apache-2.0
#include <app/exec.h>

#include <app/private/exec_internal.h>

#include <app/loader.h>
#include <app/location.h>
#include <app/manager.h>
#include <app/manifest.h>

#include <service/instance.h>
#include <service/manager.h>

#include <tactility/concurrent/mutex.h>
#include <tactility/filesystem/file_system.h>
#include <tactility/log.h>
#include <tactility/paths.h>

#include <climits>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unistd.h>
#include <unordered_map>

constexpr auto* TAG = "app_exec";

namespace {

struct ExecPathRegistry {
    std::string paths[APP_EXEC_MAX_PATHS];
    size_t count = 0;
    Mutex mutex {};

    ExecPathRegistry() { mutex_construct(&mutex); }
};

ExecPathRegistry& registry() {
    static ExecPathRegistry instance;
    return instance;
}

// POSIX works with relative paths. The rest of the OS uses anchors relative paths.
// Resolving through getcwd() keeps every path callers pass in comparable, regardless of which convention they used.
// A path already starting with '/' passes through unchanged, so this is a no-op on ESP32.
std::string make_absolute(const std::string& path) {
    if (path.empty() || path.front() == '/') {
        return path;
    }
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == nullptr) {
        return path; // best effort: falls back to today's (broken but not worse) behavior
    }
    return std::string(cwd) + "/" + path;
}

// Resolves to absolute (see make_absolute()) and strips a trailing '/' so "/data/bin/" and
// "/data/bin" register/compare identically.
std::string normalize(const char* path) {
    std::string result = make_absolute(path);
    while (result.size() > 1 && result.back() == '/') {
        result.pop_back();
    }
    return result;
}

bool has_dotdot_segment(const std::string& path) {
    size_t start = 0;
    while (start <= path.size()) {
        size_t slash = path.find('/', start);
        size_t length = (slash == std::string::npos ? path.size() : slash) - start;
        if (path.compare(start, length, "..") == 0) {
            return true;
        }
        if (slash == std::string::npos) {
            break;
        }
        start = slash + 1;
    }
    return false;
}

// True when @a file_path lives strictly under @a dir (a direct or nested child). Never equal to
// @a dir itself: a bare directory is never itself an executable candidate.
bool is_under_directory(const std::string& file_path, const std::string& dir) {
    if (file_path.size() <= dir.size() || file_path.compare(0, dir.size(), dir) != 0) {
        return false;
    }
    return file_path[dir.size()] == '/';
}

const AppLoaderApi* find_path_loader_api() {
    ServiceInstance* instance = service_manager_find_instance(APP_LOADER_PATH_SERVICE_ID);
    if (instance == nullptr) {
        return nullptr;
    }
    return static_cast<const AppLoaderApi*>(service_instance_get_data(instance));
}

// region Ad hoc "run this path directly" registry: owns the AppManifest (and its id/name/path
// strings) that app_manager's own ledger only keeps a non-owning pointer to (see
// app_manager_add()'s contract; install.cpp's InstallRegistry exists for the same reason).
// Keyed by path so re-running the same file reuses its manifest instead of re-adding it.

struct RunRecord {
    std::string id;
    std::string name;
    std::string path;
    AppManifest manifest {};
};

struct RunRegistry {
    std::unordered_map<std::string, std::unique_ptr<RunRecord>> apps;
    Mutex mutex {};

    RunRegistry() { mutex_construct(&mutex); }
};

RunRegistry& run_registry() {
    static RunRegistry registry;
    return registry;
}

std::string last_path_segment(const std::string& path) {
    auto index = path.find_last_of('/');
    return index == std::string::npos ? path : path.substr(index + 1);
}

// A short, stable-per-path id that fits AppManifest::id's APP_ID_LENGTH limit regardless of how
// long @a path is. FNV-1a's collision odds are irrelevant at the scale of "distinct executables
// one user runs from the Files app".
std::string make_run_id(const std::string& path) {
    uint32_t hash = 2166136261u;
    for (unsigned char c: path) {
        hash ^= c;
        hash *= 16777619u;
    }
    char id[16];
    std::snprintf(id, sizeof(id), "run.%08x", static_cast<unsigned int>(hash));
    return id;
}

} // namespace

extern "C" {

error_t app_exec_path_add(const char* path) {
    auto normalized = normalize(path);

    auto& reg = registry();
    mutex_lock(&reg.mutex);

    for (size_t i = 0; i < reg.count; i++) {
        if (reg.paths[i] == normalized) {
            mutex_unlock(&reg.mutex);
            return ERROR_NONE;
        }
    }

    if (reg.count >= APP_EXEC_MAX_PATHS) {
        mutex_unlock(&reg.mutex);
        LOG_E(TAG, "Registry full, can't add %s", path);
        return ERROR_OUT_OF_MEMORY;
    }

    reg.paths[reg.count++] = normalized;
    mutex_unlock(&reg.mutex);
    return ERROR_NONE;
}

error_t app_exec_path_remove(const char* path) {
    auto normalized = normalize(path);

    auto& reg = registry();
    mutex_lock(&reg.mutex);

    for (size_t i = 0; i < reg.count; i++) {
        if (reg.paths[i] == normalized) {
            reg.paths[i] = reg.paths[reg.count - 1];
            reg.count--;
            mutex_unlock(&reg.mutex);
            return ERROR_NONE;
        }
    }

    mutex_unlock(&reg.mutex);
    return ERROR_NOT_FOUND;
}

bool app_exec_path_allowed(const char* file_path) {
    std::string path = make_absolute(file_path);
    if (has_dotdot_segment(path)) {
        return false;
    }

    auto& reg = registry();
    mutex_lock(&reg.mutex);
    bool allowed = false;
    for (size_t i = 0; i < reg.count; i++) {
        if (is_under_directory(path, reg.paths[i])) {
            allowed = true;
            break;
        }
    }
    mutex_unlock(&reg.mutex);
    return allowed;
}

bool app_exec_is_executable_path(const char* path) {
    const AppLoaderApi* loader = find_path_loader_api();
    if (loader == nullptr || loader->is_executable == nullptr) {
        return false;
    }

    AppLocation location { APP_LOCATION_PATH, const_cast<char*>(path) };
    return loader->is_executable(location);
}

error_t app_exec_run_path(const char* path, int argc, const char* const argv[], AppInstanceId* out_app_instance_id) {
    if (!app_exec_is_executable_path(path)) {
        return ERROR_NOT_ALLOWED;
    }

    std::string path_str(path);
    auto& reg = run_registry();
    mutex_lock(&reg.mutex);

    auto iterator = reg.apps.find(path_str);
    std::string id;
    if (iterator != reg.apps.end()) {
        id = iterator->second->id;
        mutex_unlock(&reg.mutex);
    } else {
        auto record = std::make_unique<RunRecord>();
        record->id = make_run_id(path_str);
        record->name = last_path_segment(path_str);
        record->path = path_str;
        record->manifest = AppManifest {
            .id = record->id.c_str(),
            .name = record->name.c_str(),
            .category = APP_CATEGORY_USER,
            .location = { APP_LOCATION_PATH, const_cast<char*>(record->path.c_str()) },
            .flags = APP_MANIFEST_FLAG_HIDDEN,
            .stack = { .depth = 0, .desired_memory_capability = 0 },
        };

        error_t add_result = app_manager_add(&record->manifest);
        if (add_result != ERROR_NONE) {
            mutex_unlock(&reg.mutex);
            LOG_E(TAG, "Failed to register %s: %s", path, error_to_string(add_result));
            return add_result;
        }

        id = record->id;
        reg.apps[path_str] = std::move(record);
        mutex_unlock(&reg.mutex);
    }

    return app_manager_start_with_parameters(id.c_str(), argc, argv, out_app_instance_id);
}

void app_exec_register_default_paths() {
    char data_root[192];
    if (paths_get_data_path(data_root, sizeof(data_root)) == ERROR_NONE) {
        app_exec_path_add((std::string(data_root) + "/bin").c_str());
        app_exec_path_add((std::string(data_root) + "/app").c_str());
    } else {
        LOG_W(TAG, "Failed to resolve data path, not registering its executable directories");
    }
}

} // extern "C"
